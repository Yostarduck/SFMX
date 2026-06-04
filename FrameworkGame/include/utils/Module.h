#pragma once

#include "core/platform/PlatformDefines.h"

namespace sfmx
{

template<class T>
class Module
{
 public:
  static T& instance() {
    if (isShutDown()) {
      throw "Trying to access a module but it hasn't been started.";
    }

    if (isDestroyed()) {
      throw "Trying to access a destroyed module.";
    }
    return *_instance();
  }

  static T* instancePtr() {
    if (isShutDown()) {
      throw "Trying to access a module but it hasn't been started.";
    }

    if (isDestroyed()) {
      throw "Trying to access a destroyed module.";
    }
    return _instance();
  }
  
  template<class... Args> static void startUp(Args&& ...args) {
    if (!isShutDown()) {
      throw "Trying to start an already started module.";
    }

    _instance() = new T(std::forward<Args>(args)...);
    isShutDown() = false;

    static_cast<Module*>(_instance())->onStartUp();
  }

  template<class SubType, class... Args> static void startUp(Args&& ...args) {
    static_assert(std::is_base_of<T, SubType>::value,
      "Provided type isn't derived from type the Module is initialized with.");

    if (!isShutDown()) {
      throw "Trying to start an already started module.";
    }

    _instance() = new SubType(std::forward<Args>(args)...);
    isShutDown() = false;

    static_cast<Module*>(_instance())->onStartUp();
  }

  static void shutDown() {
    if (isShutDown() || isDestroyed()) {
      throw "Trying to shut down an already shut down module.";
    }

    static_cast<Module*>(_instance())->onShutDown();

    delete(_instance());
    isShutDown() = true;
  }

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

  virtual void onStartUp() {}

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

}