#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <type_traits>

#include "core/platform/Prerequisites.h"
#include "utils/Module.h"

namespace sfmx
{

/**
 * @brief Single-frame arena allocator (bump / stack allocator).
 *
 * Reserves one contiguous block of aligned storage at @ref startUp and hands out
 * slices of it with a monotonically advancing watermark. Individual allocations
 * are never released; @ref endFrame simply rewinds the watermark to the start of
 * the block, making every allocation from that frame reclaimable in O(1).
 *
 * Use it for per-frame transient data (temp vectors, strings, math results)
 * that is discarded at the end of the frame and needs no destructor. The arena
 * is a @ref Module singleton mirroring @ref MemoryPoolHandler, started once
 * before the first user and shut down after the last.
 *
 * @note The arena holds raw memory only — it never tracks or invokes
 *       destructors. Only allocate types whose destruction is a no-op
 *       (@c std::is_trivially_destructible) or raw bytes. The typed helpers
 *       @ref alloc / @ref allocZero enforce this at compile time.
 */
class SFMX_UTILITY_EXPORT FrameMemory : public Module<FrameMemory>
{
 public:
  /**
   * @brief Reserve @p capacityBytes of aligned backing storage (startUp).
   *
   * No elements are constructed; the arena starts empty (watermark 0). The
   * capacity is stored on construction (startUp forwards its args to T's ctor);
   * onStartUp allocates the actual block.
   */
  void onStartUp() override;

  /** @brief Release the backing storage. All arena pointers become invalid. */
  void onShutDown() override;

  /**
   * @brief Reclaim every allocation made since the last @ref endFrame, O(1).
   *
   * The storage is NOT zeroed and no destructors run: bytes written this frame
   * remain until overwritten by a future frame's allocations. Call exactly once
   * per frame, from the frame owner (the game loop).
   */
  void endFrame();

  /**
   * @brief Reserve @p size bytes from the arena, aligned to @p align.
   *
   * @p align must be a nonzero power of two no larger than @c 64.
   *
   * @return Pointer to the reserved range, or nullptr if the arena is exhausted
   *         (placeholder behaviour; calls fall back to the heap or skip the
   *         frame). Exhaustion is logged, not asserted: it is an expected
   *         runtime condition, not a programmer error.
   */
  NODISCARD void*
  allocate(size_t size, size_t align = alignof(std::max_align_t));

  /** @brief Like @ref allocate, but zero-fills the reserved range first. */
  NODISCARD void*
  allocateZero(size_t size, size_t align = alignof(std::max_align_t));

  /**
   * @brief Reserve room for @p count @p T objects.
   *
   * @tparam T Must be trivially destructible (the arena never runs destructors).
   */
  template<typename T>
  NODISCARD T*
  alloc(size_t count = 1) {
    static_assert(std::is_trivially_destructible<T>::value,
                  "FrameMemory only supports trivially destructible types");
    return static_cast<T*>(allocate(sizeof(T) * count, alignof(T)));
  }

  /** @brief Like @ref alloc, but zero-fills the reserved range first. */
  template<typename T>
  NODISCARD T*
  allocZero(size_t count = 1) {
    static_assert(std::is_trivially_destructible<T>::value,
                  "FrameMemory only supports trivially destructible types");
    return static_cast<T*>(allocateZero(sizeof(T) * count, alignof(T)));
  }

  /** @brief Bytes reserved so far this frame (before alignment padding). */
  NODISCARD FORCEINLINE size_t getUsedBytes() const { return m_offset; }

  /** @brief Total bytes of backing storage reserved at startUp. */
  NODISCARD FORCEINLINE size_t getCapacity() const { return m_capacity; }

  /** @brief Bytes still available before the arena is exhausted. */
  NODISCARD FORCEINLINE size_t getAvailableBytes() const {
    return m_capacity - m_offset;
  }

  /** @brief True once the arena has no contiguous room left. */
  NODISCARD FORCEINLINE bool isExhausted() const { return m_offset >= m_capacity; }

 protected:
  friend class Module<FrameMemory>;

  explicit FrameMemory(size_t capacityBytes = 0)
    : m_capacity(capacityBytes)
  {}

 private:
  /** @brief Round @p value up to a multiple of @p alignment (power of two). */
  NODISCARD FORCEINLINE static size_t
  alignUp(size_t value, size_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
  }

  uint8* m_memory   = nullptr;
  size_t m_capacity = 0;
  size_t m_offset   = 0;
};

} // namespace sfmx
