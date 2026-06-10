#pragma once

#include "core/MemoryPool.h"
#include "core/platform/Prerequisites.h"
#include "utils/Module.h"
#include "utils/TypeTraits.h"
#include "utils/UUID.h"

namespace sfmx
{

/**
 * @brief Type-erased handle to a MemoryPool.
 *
 * Lets @ref MemoryPoolHandler keep pools of different element types in one
 * container and drive them (deallocate, clear, query) without knowing the
 * concrete type and without RTTI.
 */
class IMemoryPool
{
 public:
  virtual ~IMemoryPool() = default;

  /**
   * @brief Destroy an object previously allocated from this pool and free its
   *        slot, given only its address.
   *
   * @param object Address of a live element of this pool's element type. For
   *               the framework's single-public-inheritance, offset-zero
   *               hierarchies (see CodeStyle: composition first, public
   *               inheritance only, no RTTI) a base-subobject pointer is a valid
   *               argument, so a @c Component* may be passed for a component
   *               pool.
   */
  virtual void deallocate(void* object) = 0;

  /**
   * @brief Destroy every live element and return all slots to free, keeping the
   *        backing storage and capacity (the pool stays usable, just empty).
   */
  virtual void clear() = 0;

  /** @brief Total number of slots (the capacity fixed at registration). */
  NODISCARD virtual size_t getCapacity() const = 0;
  /** @brief Number of slots currently in use. */
  NODISCARD virtual size_t getAllocatedCount() const = 0;
  /** @brief Bytes of backing storage this pool owns (elements + bit flags). */
  NODISCARD virtual size_t getMemoryUsage() const = 0;
};

/**
 * @brief Owns one @ref MemoryPool of element type @p T behind the
 *        @ref IMemoryPool interface.
 */
template<typename T>
class TypedPool : public IMemoryPool
{
 public:
  explicit TypedPool(size_t capacity) { m_pool.initialize(capacity); }

  NODISCARD MemoryPool<T>&
  pool() { return m_pool; }

  void
  deallocate(void* object) override {
    m_pool.deallocate(static_cast<T*>(object));
  }

  void
  clear() override { m_pool.clear(); }

  NODISCARD size_t
  getCapacity() const override { return m_pool.getCapacity(); }

  NODISCARD size_t
  getAllocatedCount() const override { return m_pool.getAllocatedCount(); }

  NODISCARD size_t
  getMemoryUsage() const override { return m_pool.getMemoryUsage(); }

 private:
  MemoryPool<T> m_pool;
};

/**
 * @brief Process-wide owner of every fixed-size @ref MemoryPool in the
 *        framework.
 *
 * A @ref Module (CRTP singleton) so any subsystem can register a pool for its
 * own type and pull pooled storage from a globally reachable place. Pools are
 * keyed by type id (@ref TypeTraits::getTypeId), one per registered type, and
 * are fixed-size: size each pool generously at @ref registerPool time.
 *
 * Because the handler is a single process-wide instance, its pools are shared
 * across everything that uses them (e.g. all Scenes draw their SceneNode and
 * component storage from here). Start it up before the first user, register the
 * needed pools, and shut it down after the last user is gone.
 */
class MemoryPoolHandler : public Module<MemoryPoolHandler>
{
 public:
  /**
   * @brief Create the pool for type @p T with its own fixed @p capacity.
   *
   * Call once per type during setup, before allocating any @p T. Asserts if a
   * pool for @p T already exists.
   */
  template<typename T>
  void registerPool(size_t capacity);

  /** @brief Whether a pool for type @p T has been registered. */
  template<typename T>
  NODISCARD bool hasPool() const;

  /**
   * @brief Access the live @ref MemoryPool for type @p T for direct
   *        allocate / deallocate.
   *
   * If @p T was never registered, lazily creates the pool at the default
   * capacity and asserts so the omission is caught in debug builds.
   */
  template<typename T>
  NODISCARD MemoryPool<T>& pool();

  /**
   * @brief Destroy every live element of type @p T, keeping the pool's storage
   *        and capacity so it is immediately reusable (e.g. between scenes).
   */
  template<typename T>
  void clear();

  /** @brief Capacity of type @p T's pool, or 0 if it is not registered. */
  template<typename T>
  NODISCARD size_t getCapacity() const;

  /** @brief Live element count of type @p T's pool, or 0 if not registered. */
  template<typename T>
  NODISCARD size_t getAllocatedCount() const;

  /** @brief Backing bytes of type @p T's pool, or 0 if it is not registered. */
  template<typename T>
  NODISCARD size_t getMemoryUsage() const;

  /**
   * @brief Erased teardown path: destroy @p object in the pool keyed by
   *        @p typeId. No-op if @p typeId is unknown.
   *
   * Used where the concrete type is not known at the call site (e.g. destroying
   * a node whose components are only reachable through a @c Component* list).
   */
  void deallocate(const UUID& typeId, void* object);

  /** @brief @ref clear every registered pool, leaving them registered/sized. */
  void reset();

  /** @brief Sum of every pool's backing bytes: the whole pooled footprint. */
  NODISCARD size_t getTotalMemoryUsage() const;

  void setDefaultCapacity(size_t capacity) { m_defaultCapacity = capacity; }
  NODISCARD size_t getDefaultCapacity() const { return m_defaultCapacity; }

 protected:
  /** @brief Release every pool (each pool destroys its live elements). */
  void onShutDown() override;

 private:
  friend class Module<MemoryPoolHandler>;

  explicit MemoryPoolHandler(size_t defaultCapacity = 64)
    : m_defaultCapacity(defaultCapacity)
  {}

  UnorderedMap<UUID, UniquePtr<IMemoryPool>> m_pools;
  size_t m_defaultCapacity;
};

// ---------------------------------------------------------------------------
// Template definitions.
// ---------------------------------------------------------------------------

template<typename T>
void
MemoryPoolHandler::registerPool(size_t capacity) {
  const UUID& id = TypeTraits<T>::getTypeId();
  SFMX_ASSERT(m_pools.find(id) == m_pools.end());
  m_pools[id] = UniquePtr<IMemoryPool>(new TypedPool<T>(capacity));
}

template<typename T>
NODISCARD bool
MemoryPoolHandler::hasPool() const {
  return m_pools.find(TypeTraits<T>::getTypeId()) != m_pools.end();
}

template<typename T>
NODISCARD MemoryPool<T>&
MemoryPoolHandler::pool() {
  const UUID& id = TypeTraits<T>::getTypeId();
  const auto it = m_pools.find(id);
  if (it != m_pools.end()) {
    return static_cast<TypedPool<T>*>(it->second.get())->pool();
  }

  // Not registered: create at the default capacity, but assert so a missing
  // registerPool<T>() shows up in debug builds.
  SFMX_ASSERT(false && "pool type used without registerPool<T>()");
  TypedPool<T>* created = new TypedPool<T>(m_defaultCapacity);
  m_pools[id] = UniquePtr<IMemoryPool>(created);
  return created->pool();
}

template<typename T>
void
MemoryPoolHandler::clear() {
  const auto it = m_pools.find(TypeTraits<T>::getTypeId());
  if (it != m_pools.end()) {
    it->second->clear();
  }
}

template<typename T>
NODISCARD size_t
MemoryPoolHandler::getCapacity() const {
  const auto it = m_pools.find(TypeTraits<T>::getTypeId());
  return (it != m_pools.end()) ? it->second->getCapacity() : 0;
}

template<typename T>
NODISCARD size_t
MemoryPoolHandler::getAllocatedCount() const {
  const auto it = m_pools.find(TypeTraits<T>::getTypeId());
  return (it != m_pools.end()) ? it->second->getAllocatedCount() : 0;
}

template<typename T>
NODISCARD size_t
MemoryPoolHandler::getMemoryUsage() const {
  const auto it = m_pools.find(TypeTraits<T>::getTypeId());
  return (it != m_pools.end()) ? it->second->getMemoryUsage() : 0;
}

}  // namespace sfmx
