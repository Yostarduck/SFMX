#pragma once


/************************************************************************/
/*
 * Threading
 */
/************************************************************************/
#include <mutex>
#include <thread>

namespace sfmx
{

/************************************************************************/
/*
 * Threading
 */
 /************************************************************************/

/**
 * @brief Wrapper for the C++ std::recursive_mutex.
 */
using RecursiveMutex = std::recursive_mutex;

/**
 * @brief Wrapper for the C++ std::unique_lock<std::recursive_mutex>.
 */
using RecursiveLock = std::unique_lock<RecursiveMutex>;


/**
 * @brief Wrapper for the C++ std::thread.
 */
using Thread = std::thread;

/**
 * @brief Wrapper for the C++ mutex.
 */
using Mutex = std::mutex;

/**
 * @brief Wrapper for lock_guard.
 */
template<typename Mutex>
using LockGuard = std::lock_guard<Mutex>;
}

