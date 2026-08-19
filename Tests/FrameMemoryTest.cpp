#include <doctest/doctest.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "core/platform/Prerequisites.h"
#include "utils/FrameMemory.h"

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

TEST_CASE("FrameMemory allocates aligned slices and bumps the watermark") {
  FrameScope scope;

  FrameMemory& mem = FrameMemory::instance();
  CHECK(mem.getUsedBytes() == 0);

  void* a = mem.allocate(64);
  REQUIRE(a != nullptr);
  CHECK(reinterpret_cast<uintptr_t>(a) % alignof(std::max_align_t) == 0);

  void* b = mem.allocate(64);
  REQUIRE(b != nullptr);
  CHECK(a != b);  // distinct, monotonically bumped slices
  CHECK(reinterpret_cast<uintptr_t>(b) >=
        reinterpret_cast<uintptr_t>(a) + 64);
  CHECK(mem.getUsedBytes() >= 128);
}

TEST_CASE("FrameMemory honours explicit alignments") {
  FrameScope scope;

  FrameMemory& mem = FrameMemory::instance();
  const std::array<size_t, 4> alignments = {1u, 8u, 16u, 64u};
  for (const size_t align : alignments) {
    void* p = mem.allocate(5, align);
    REQUIRE(p != nullptr);
    CHECK(reinterpret_cast<uintptr_t>(p) % align == 0);
  }
}

TEST_CASE("FrameMemory endFrame rewinds the arena and reuses its start") {
  FrameScope scope;

  FrameMemory& mem = FrameMemory::instance();
  void* first = mem.allocate(32);
  REQUIRE(first != nullptr);
  REQUIRE(mem.getUsedBytes() > 0);

  mem.endFrame();
  CHECK(mem.getUsedBytes() == 0);
  CHECK_FALSE(mem.isExhausted());

  // A fresh frame's first allocation lands back at the block start.
  void* second = mem.allocate(32);
  REQUIRE(second != nullptr);
  CHECK(second == first);
}

TEST_CASE("FrameMemory returns nullptr when exhausted") {
  FrameScope scope(128);

  FrameMemory& mem = FrameMemory::instance();
  void* a = mem.allocate(96);
  REQUIRE(a != nullptr);
  CHECK(mem.getAvailableBytes() == 128 - 96);

  // 96 was taken; 64 more must not fit in the 128-byte arena.
  void* b = mem.allocate(64);
  CHECK(b == nullptr);
}

TEST_CASE("FrameMemory alloc<T> constructs trivially destructible types") {
  FrameScope scope;

  FrameMemory& mem = FrameMemory::instance();
  int* ints = mem.alloc<int>(4);
  REQUIRE(ints != nullptr);
  ints[0] = 1; ints[1] = 2; ints[2] = 3; ints[3] = 4;
  CHECK(ints[3] == 4);

  // The arena only accepts trivially destructible types.
  static_assert(std::is_trivially_destructible<int>::value);
  static_assert(!std::is_trivially_destructible<std::string>::value);
}

TEST_CASE("FrameMemory allocZero zero-fills the range") {
  FrameScope scope;

  FrameMemory& mem = FrameMemory::instance();
  uint8* bytes = mem.allocZero<uint8>(64);
  REQUIRE(bytes != nullptr);
  for (size_t i = 0; i < 64; ++i) {
    CHECK(bytes[i] == 0);
  }

  // Pre-touch the arena with garbage so a reallocated slice is visibly non-zero.
  mem.endFrame();
  std::memset(mem.allocate(64), 0xAB, 64);
  mem.endFrame();

  uint8* zeroed = mem.allocZero<uint8>(64);
  REQUIRE(zeroed != nullptr);
  for (size_t i = 0; i < 64; ++i) {
    CHECK(zeroed[i] == 0);
  }
}

TEST_CASE("FrameMemory module restarts cleanly") {
  FrameScope scope(256);

  FrameMemory& mem = FrameMemory::instance();
  void* p = mem.allocate(16);
  REQUIRE(p != nullptr);

  FrameMemory::shutDown();
  REQUIRE_FALSE(FrameMemory::isStarted());

  FrameMemory::startUp(256);
  REQUIRE(FrameMemory::isStarted());
  CHECK(FrameMemory::instance().getUsedBytes() == 0);
  // The old pointer is dangling after shutdown; a fresh allocation works.
  void* q = FrameMemory::instance().allocate(16);
  REQUIRE(q != nullptr);
}
