#pragma once

#include <new>

#include "core/platform/Prerequisites.h"

namespace sfmx
{

/**
 * @brief Fixed-size object pool (Object Pool pattern, Robert Nystrom -
 *        "Game Programming Patterns").
 *
 * Reserves a contiguous block of raw, suitably-aligned storage for @p T
 * elements and constructs them on demand: @ref allocate placement-constructs an
 * element in a free slot, @ref deallocate destroys it. This avoids per-element
 * heap traffic while keeping element lifetimes tied to (de)allocation rather
 * than to the pool itself. Occupancy is tracked with a bit field
 * (@ref m_allocatedBitFlags): every byte of that field tracks 8 elements, one
 * bit each (1 = in use, 0 = free).
 *
 * The pool is non-copyable; it owns the backing storage.
 */
template<typename T>
class MemoryPool
{
 public:
  MemoryPool()
    : m_poolSize(0),
      m_allocatedBitFlags(nullptr),
      m_elements(nullptr),
      m_freeElements(0)
  {}

  ~MemoryPool() { freePool(); }

  MemoryPool(const MemoryPool&) = delete;
  MemoryPool& operator=(const MemoryPool&) = delete;

  /**
   * @brief Reserve raw storage for @p poolSize elements.
   *
   * Any storage held from a previous call is released first. No elements are
   * constructed here - the storage is left uninitialised and every slot starts
   * out marked as free; elements are built on demand by @ref allocate.
   *
   * @param poolSize Number of elements the pool can hold.
   */
  void
  initialize(size_t poolSize) {
    freePool();

    if (0 == poolSize) {
      return;
    }

    m_poolSize = poolSize;
    m_freeElements = static_cast<int32>(poolSize);

    // Raw, suitably-aligned storage; elements are placement-constructed later.
    m_elements = static_cast<T*>(::operator new(poolSize * sizeof(T),
                                                std::align_val_t{alignof(T)}));

    // One bit per element, packed 8 elements per byte; round up.
    const size_t flagBytes = flagByteCount(poolSize);
    m_allocatedBitFlags = new int8[flagBytes];
    for (size_t i = 0; i < flagBytes; ++i) {
      m_allocatedBitFlags[i] = 0;
    }
  }

  /**
   * @brief Construct an element in the next free slot and mark it as in use.
   *
   * The element is placement-constructed in place, forwarding @p args to its
   * constructor. If that constructor throws, the slot is left free and the
   * exception propagates.
   *
   * @tparam Args Constructor argument types for @p T.
   * @param  args Arguments forwarded to @p T's constructor.
   * @return Pointer to the new element, or nullptr if the pool is full.
   */
  template<typename... Args>
  NODISCARD T*
  allocate(Args&&... args) {
    if (isFull()) {
      return nullptr;
    }

    const size_t index = findFreeIndex();
    if (index >= m_poolSize) {
      return nullptr;
    }

    T* slot = &m_elements[index];
    ::new (static_cast<void*>(slot)) T(std::forward<Args>(args)...);

    setElementUsed(index);
    --m_freeElements;
    return slot;
  }

  /**
   * @brief Destroy an element and return its slot to the pool.
   *
   * Ignores null pointers and pointers that do not belong to this pool, as
   * well as slots that are already free.
   *
   * @param element Pointer previously returned by @ref allocate.
   */
  void
  deallocate(T* element) {
    if (nullptr == element || nullptr == m_elements) {
      return;
    }

    // Reject pointers outside the backing storage.
    if (element < m_elements || element >= m_elements + m_poolSize) {
      return;
    }

    const size_t index = static_cast<size_t>(element - m_elements);
    if (!isElementUsed(index)) {
      return;  // double free / already available
    }

    element->~T();
    setElementFree(index);
    ++m_freeElements;
  }

  /**
   * @brief Destroy any live elements, release the backing storage and reset the
   *        pool to an empty state.
   */
  void
  freePool() {
    if (nullptr != m_elements) {
      // Destroy elements that are still in use before reclaiming the storage.
      for (size_t i = 0; i < m_poolSize; ++i) {
        if (isElementUsed(i)) {
          m_elements[i].~T();
        }
      }
      ::operator delete(static_cast<void*>(m_elements),
                        std::align_val_t{alignof(T)});
    }

    delete[] m_allocatedBitFlags;
    m_elements = nullptr;
    m_allocatedBitFlags = nullptr;
    m_poolSize = 0;
    m_freeElements = 0;
  }

  /**
   * @brief Whether every element is currently in use.
   */
  NODISCARD bool
  isFull() const {
    return 0 == m_freeElements;
  }

  /**
   * @brief Address of the slot the next @ref allocate would use, without
   *        claiming it.
   *
   * The slot holds raw, uninitialised storage - no element has been constructed
   * there yet, so the result must not be dereferenced until a matching
   * @ref allocate constructs an object in it.
   *
   * @return Address of the first free slot, or nullptr if the pool is full.
   */
  NODISCARD T*
  getNextElement() {
    if (isFull()) {
      return nullptr;
    }

    const size_t index = findFreeIndex();
    if (index >= m_poolSize) {
      return nullptr;
    }

    return &m_elements[index];
  }

 private:
  /**
   * @brief Number of flag bytes needed to track @p poolSize elements (8 per byte).
   */
  NODISCARD static size_t
  flagByteCount(size_t poolSize) {
    return (poolSize + 7) >> 3;
  }

  /**
   * @brief Test whether the element at @p index is marked as in use.
   */
  NODISCARD bool
  isElementUsed(size_t index) const {
    const size_t byteIndex = index >> 3;          // index / 8
    const int8 mask = static_cast<int8>(1 << (index & 7));  // 1 << (index % 8)
    return 0 != (m_allocatedBitFlags[byteIndex] & mask);
  }

  /**
   * @brief Mark the element at @p index as in use.
   */
  void
  setElementUsed(size_t index) {
    const size_t byteIndex = index >> 3;
    const int8 mask = static_cast<int8>(1 << (index & 7));
    m_allocatedBitFlags[byteIndex] |= mask;
  }

  /**
   * @brief Mark the element at @p index as free.
   */
  void
  setElementFree(size_t index) {
    const size_t byteIndex = index >> 3;
    const int8 mask = static_cast<int8>(1 << (index & 7));
    m_allocatedBitFlags[byteIndex] &= static_cast<int8>(~mask);
  }

  /**
   * @brief Find the index of the first free element.
   *
   * Whole bytes equal to 0xFF are skipped, then the free bit within the first
   * non-full byte is located.
   *
   * @return Index of the first free element, or @ref m_poolSize if none.
   */
  NODISCARD size_t
  findFreeIndex() const {
    const size_t flagBytes = flagByteCount(m_poolSize);
    for (size_t byteIndex = 0; byteIndex < flagBytes; ++byteIndex) {
      if (0xFFu == static_cast<uint8>(m_allocatedBitFlags[byteIndex])) {
        continue;  // all 8 slots in this byte are taken
      }

      for (size_t bit = 0; bit < 8; ++bit) {
        const size_t index = (byteIndex << 3) + bit;
        if (index >= m_poolSize) {
          break;  // padding bits past the end of the pool
        }
        if (!isElementUsed(index)) {
          return index;
        }
      }
    }

    return m_poolSize;
  }

  size_t m_poolSize;

  uint8* m_allocatedBitFlags;
  T* m_elements;

  uint32 m_freeElements;
};

}