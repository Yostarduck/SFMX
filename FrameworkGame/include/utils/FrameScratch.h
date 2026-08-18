#pragma once

#include <array>
#include <cstddef>
#include <type_traits>
#include <vector>

#include "core/platform/Prerequisites.h"
#include "utils/FrameMemory.h"

namespace sfmx
{

/**
 * @brief Bounded, per-frame scratch buffer for trivially destructible types.
 *
 * Backs small counts with a fixed inline array (no allocation at all) and
 * falls back to @ref FrameMemory when the count exceeds the stack capacity.
 * The storage is only valid until the next @c endFrame(); use it for data
 * that is produced and consumed within a single frame.
 *
 * @tparam T            Trivially destructible element type.
 * @tparam StackCapacity Number of elements kept inline on the stack.
 */
template<typename T, size_t StackCapacity>
class FrameScratch
{
  static_assert(std::is_trivially_destructible<T>::value,
                "FrameScratch only supports trivially destructible types");

 public:
  /**
   * @brief Reserve room for @p count elements.
   * @note Requires @ref FrameMemory to be started once @p count exceeds
   *       @p StackCapacity. If the arena is exhausted the scratch falls back
   *       to a heap vector so callers never write past the inline stack array.
   */
  explicit FrameScratch(size_t count) {
    m_count = count;
    if (count > StackCapacity) {
      m_data = FrameMemory::instance().alloc<T>(count);
      if (nullptr == m_data) {
        // Arena exhausted (recoverable): heap fallback keeps the buffer valid.
        m_heap.resize(count);
        m_data = m_heap.data();
      }
    }
  }

  /** @brief Pointer to the first element (stack or arena backed). */
  NODISCARD FORCEINLINE T* data() {
    return (m_data != nullptr) ? m_data : m_stack.data();
  }

  /** @brief Pointer to the first element (stack or arena backed). */
  NODISCARD FORCEINLINE const T* data() const {
    return (m_data != nullptr) ? m_data : m_stack.data();
  }

  /** @brief Number of elements reserved. */
  NODISCARD FORCEINLINE size_t size() const { return m_count; }

  /** @brief Iterator to the first element, usable with std::sort. */
  NODISCARD FORCEINLINE T* begin() { return data(); }

  /** @brief Iterator one past the last element, usable with std::sort. */
  NODISCARD FORCEINLINE T* end() { return data() + m_count; }

  /** @brief Bounds-checked element access. */
  NODISCARD FORCEINLINE T& operator[](size_t index) {
    return data()[index];
  }

  /** @brief Bounds-checked element access (const). */
  NODISCARD FORCEINLINE const T& operator[](size_t index) const {
    return data()[index];
  }

 private:
  // Only indices < m_count are ever read, and every caller writes them before
  // reading, so the stack array is deliberately left uninitialized.
  Array<T, StackCapacity> m_stack;
  Vector<T>               m_heap;
  T*     m_data = nullptr;
  size_t m_count = 0;
};

} // namespace sfmx
