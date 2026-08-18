#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/platform/Prerequisites.h"
#include "utils/FrameMemory.h"
#include "utils/FrameScratch.h"

using namespace sfmx;

// RAII: start/shutdown the frame arena even if a REQUIRE throws.
struct FrameScope {
  explicit FrameScope(size_t capacityBytes = 1024u * 1024u) {
    if (FrameMemory::isStarted()) {
      FrameMemory::shutDown();
    }
    FrameMemory::startUp(capacityBytes);
  }
  ~FrameScope() {
    if (FrameMemory::isStarted()) {
      FrameMemory::shutDown();
    }
  }
};

TEST_CASE("FrameScratch uses no arena for counts within the stack capacity") {
  FrameScope scope;

  FrameScratch<int, 8> small(4);
  CHECK(small.size() == 4);
  REQUIRE(small.data() != nullptr);
  // Entirely stack-backed: the frame arena must not have been touched.
  CHECK(FrameMemory::instance().getUsedBytes() == 0);
}

TEST_CASE("FrameScratch falls back to the frame arena past stack capacity") {
  FrameScope scope;

  constexpr size_t kStackCapacity = 8;
  constexpr size_t kCount         = 64;
  FrameScratch<uint32, kStackCapacity> big(kCount);

  // Overflow spilled into FrameMemory: count * sizeof(element).
  CHECK(FrameMemory::instance().getUsedBytes() == kCount * sizeof(uint32));

  // The buffer is viewable as a contiguous range for std::sort to consume.
  REQUIRE(big.data() != nullptr);
  for (size_t i = 0; i < kCount; ++i) {
    big[i] = static_cast<uint32>(kCount - i);  // reverse order
  }
  std::sort(big.begin(), big.end());
  CHECK(big[0] == 1);
  CHECK(big[kCount - 1] == kCount);
}

TEST_CASE("FrameScratch data never survives endFrame") {
  FrameScope scope;

  constexpr size_t kStackCapacity = 4;
  constexpr size_t kCount         = 32;

  FrameScratch<uint32, kStackCapacity> first(kCount);
  const uint32* firstStorage = first.data();
  REQUIRE(firstStorage != nullptr);
  const size_t bytesUsed = FrameMemory::instance().getUsedBytes();

  FrameMemory::instance().endFrame();
  CHECK(FrameMemory::instance().getUsedBytes() == 0);

  // A new overflow buffer this frame reuses the block from the start, so it
  // lands exactly where the previous frame's buffer was handed out.
  FrameScratch<uint32, kStackCapacity> second(kCount);
  CHECK(FrameMemory::instance().getUsedBytes() == bytesUsed);
  CHECK(second.data() == firstStorage);
}

TEST_CASE("FrameScratch sorts transient data that is relinked, like the particle sort") {
  FrameScope scope;

  // Mirrors ParticleSystemComponent::onUpdate's kBackToFront sort: a transient
  // pointer buffer produced and consumed within one frame.
  constexpr size_t kStackCapacity = 4;
  constexpr size_t kCount         = 16;

  std::vector<uint32> values(kCount);
  for (size_t i = 0; i < kCount; ++i) {
    values[i] = static_cast<uint32>(kCount - i);  // reverse order, like spawn order
  }

  FrameScratch<uint32*, kStackCapacity> sorted(kCount);
  for (size_t i = 0; i < kCount; ++i) {
    sorted[i] = values.data() + i;
  }

  std::sort(sorted.begin(), sorted.end(),
            [](const uint32* a, const uint32* b) { return *a < *b; });

  CHECK(*sorted[0] == 1);
  CHECK(*sorted[kCount - 1] == kCount);
}