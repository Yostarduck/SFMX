#pragma once

#include "core/platform/Prerequisites.h"

#include <stdexcept>

namespace sfmx
{

/**
 * @brief Base class for systems that exist as a single, globally accessible
 *        instance with an explicit start-up / shut-down lifecycle.
 *
 * Derive using the CRTP idiom (e.g. @c class Foo : public Module<Foo>). The
 * instance must be created with startUp() before it can be reached through
 * instance() / instancePtr(), and released with shutDown(). A module may be
 * started again after being shut down.
 *
 * @tparam T The concrete module type deriving from this class.
 */
template<class T>
class Module
{
 public:
  /**
   * @brief Returns a reference to the started module instance.
   * @throws std::runtime_error If the module has not been started or was destroyed.
   */
  static T& instance() {
    if (isShutDown()) {
      throw std::runtime_error("Trying to access a module but it hasn't been started.");
    }

    if (isDestroyed()) {
      throw std::runtime_error("Trying to access a destroyed module.");
    }

    return *_instance();
  }

  /**
   * @brief Returns a pointer to the started module instance.
   * @throws std::runtime_error If the module has not been started or was destroyed.
   */
  static T* instancePtr() {
    if (isShutDown()) {
      throw std::runtime_error("Trying to access a module but it hasn't been started.");
    }

    if (isDestroyed()) {
      throw std::runtime_error("Trying to access a destroyed module.");
    }

    return _instance();
  }

  /**
   * @brief Constructs and starts the module, forwarding @p args to T's constructor.
   * @throws std::runtime_error If the module is already started.
   */
  template<class... Args>
  static void startUp(Args&&... args) {
    if (!isShutDown()) {
      throw std::runtime_error("Trying to start an already started module.");
    }

    _instance() = new T(std::forward<Args>(args)...);
    isShutDown() = false;
    isDestroyed() = false;

    static_cast<Module*>(_instance())->onStartUp();
  }

  /**
   * @brief Constructs and starts the module using a derived type @p SubType.
   * @throws std::runtime_error If the module is already started.
   */
  template<class SubType, class... Args>
  static void startUp(Args&&... args) {
    static_assert(std::is_base_of_v<T, SubType>,
      "Provided type isn't derived from type the Module is initialized with.");

    if (!isShutDown()) {
      throw std::runtime_error("Trying to start an already started module.");
    }

    _instance() = new SubType(std::forward<Args>(args)...);
    isShutDown() = false;
    isDestroyed() = false;

    static_cast<Module*>(_instance())->onStartUp();
  }

  /**
   * @brief Shuts down and destroys the module instance.
   * @throws std::runtime_error If the module is not currently started.
   */
  static void shutDown() {
    if (isShutDown() || isDestroyed()) {
      throw std::runtime_error("Trying to shut down an already shut down module.");
    }

    static_cast<Module*>(_instance())->onShutDown();

    delete _instance();
    isShutDown() = true;
  }

  /**
   * @brief Returns true if the module is currently started and usable.
   */
  static bool isStarted() {
    return !isShutDown() && !isDestroyed();
  }

 protected:
  Module() = default;

  virtual ~Module() {
    _instance() = nullptr;
    isDestroyed() = true;
  }

  Module(const Module&) = delete;
  Module(Module&&) = delete;
  Module& operator=(const Module&) = delete;
  Module& operator=(Module&&) = delete;

  /**
   * @brief Called right after the instance is constructed during startUp().
   */
  virtual void onStartUp() {}

  /**
   * @brief Called right before the instance is destroyed during shutDown().
   */
  virtual void onShutDown() {}

  static T*& _instance() {
    static T* inst = nullptr;
    return inst;
  }

  static bool& isDestroyed() {
    static bool inst = false;
    return inst;
  }

  static bool& isShutDown() {
    static bool inst = true;
    return inst;
  }
};

} // namespace sfmx
