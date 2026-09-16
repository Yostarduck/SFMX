#include "utils/FrameMemory.h"

#include <cstring>
#include <iostream>

namespace sfmx
{

/** @brief Max alignment the arena honours; also the block's base alignment. */
constexpr size_t kMaxAlignment = 64;

void
FrameMemory::onStartUp() {
  SFMX_ASSERT(m_memory == nullptr);
  SFMX_ASSERT(m_offset == 0);

  if (0 == m_capacity) {
    return;
  }

  // Raw, suitably-aligned storage; slices are handed out by allocate(). The block
  // is over-aligned (and over-allocated by kMaxAlignment-1) so any power-of-two
  // request up to kMaxAlignment can be satisfied from an aligned base.
  uint8* raw = static_cast<uint8*>(
      ::operator new(m_capacity + kMaxAlignment - 1,
                     std::align_val_t{kMaxAlignment}));
  m_allocationBase  = raw;
  m_allocationSize  = m_capacity + kMaxAlignment - 1;
  m_memory          = reinterpret_cast<uint8*>(
      alignUp(reinterpret_cast<uintptr_t>(raw), kMaxAlignment));
  m_offset = 0;
}

void
FrameMemory::onShutDown() {
  if (nullptr != m_allocationBase) {
    // Delete the exact pointer new returned (m_memory may sit past the base if
    // a future alignment scheme ever bumps it), with the size it was created with.
    ::operator delete(static_cast<void*>(m_allocationBase), m_allocationSize,
                      std::align_val_t{kMaxAlignment});
    m_allocationBase = nullptr;
  }
  m_memory         = nullptr;
  m_allocationSize = 0;
  m_offset         = 0;
}

void
FrameMemory::endFrame() {
  // No destructors, no zeroing: the watermark returns to the start of the block
  // and the next frame reuses the same storage.
  m_offset = 0;
  ++m_frameCounter;
}

void*
FrameMemory::allocate(size_t size, size_t align) {
  if (0 == size) {
    return nullptr;
  }

  // The requested alignment must be a power of two within our supported range.
  SFMX_ASSERT(align != 0 && (align & (align - 1)) == 0);
  SFMX_ASSERT(align <= kMaxAlignment);

  const size_t alignedOffset = alignUp(m_offset, align);
  const size_t newOffset     = alignedOffset + size;

  if (newOffset > m_capacity) {
    // Exhaustion is a recoverable runtime condition (a frame just needs a bigger
    // arena), so log rather than abort; callers fall back to the heap or skip.
    // Report at most once per frame so a burst of failures doesn't spam the log.
    if (m_exhaustedReportedFrame != m_frameCounter) {
      std::cerr << "[FrameMemory] allocation of " << size
                << " bytes exceeds the " << m_capacity
                << "-byte arena (watermark " << m_offset
                << "); returning nullptr\n";
      m_exhaustedReportedFrame = m_frameCounter;
    }
    return nullptr;
  }

  uint8* slice = m_memory + alignedOffset;
  m_offset     = newOffset;
  return slice;
}

void*
FrameMemory::allocateZero(size_t size, size_t align) {
  void* slice = allocate(size, align);
  if (nullptr != slice) {
    std::memset(slice, 0, size);
  }
  return slice;
}

}  // namespace sfmx