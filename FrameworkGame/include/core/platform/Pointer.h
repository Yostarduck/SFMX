#pragma once

#include "STDHeaders.h"
#include <memory>

namespace sfmx
{
/************************************************************************/
/*
 * Smart pointers
 */
/************************************************************************/

/**
 *   Shared pointer that will be used for Chimera.
 **/
template <typename T>
using SPtr = std::shared_ptr<T>;

/**
 *   Weak pointer used along Chimera.
 **/
template<typename T>
using WeakPtr = std::weak_ptr<T>;

template<typename T>
struct ForwardDeleter {
    void operator()(T* ptr) const {
        delete ptr;
    }
};

/**
 *   Unique pointer used along Chimera.
 **/
template<class T>
using UniquePtr = std::unique_ptr<T, ForwardDeleter<T>>;

/**
 * @brief Create a new shared pointer using a custom allocator category.
 */
template<class T, class... Args>
SPtr<T>
MakeShared(Args&&... args) {
  return std::allocate_shared<T>(Alloc<T>(),
                                 std::forward<Args>(args)...);
}

/**
 * @brief Create a new shared pointer using a custom allocator category.
 */
template<class T, class... Args>
UniquePtr<T> MakeUnique(Args&&... args) {
    return UniquePtr<T>(new T(std::forward<Args>(args)...));
}


}