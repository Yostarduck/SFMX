#pragma once

#include <mutex>
#include <thread>
#include <atomic>

namespace sfmx
{
/**
 * Threading aliases kept out of the common prerequisites so the heavy threading
 * includes only reach the few files that spawn workers or guard shared state.
 */

using Mutex = std::mutex;

using RecursiveMutex = std::recursive_mutex;

using RecursiveLock = std::unique_lock<RecursiveMutex>;

template<typename Mutex>
using LockGuard = std::lock_guard<Mutex>;

using Thread = std::thread;

template<typename T>
using Atomic = std::atomic<T>;
} // namespace sfmx
