#pragma once

/************************************************************************/
/*
 * C type objects
 */
/************************************************************************/
#include <cassert>
#include <cmath>
#include <limits>

/************************************************************************/
/*
 * Types.
 */
/************************************************************************/
#include <type_traits>

/************************************************************************/
/*
 * STL Containers.
 */
/************************************************************************/
#include <array>
#include <vector>
#include <queue>

#include <ranges>

/************************************************************************/
/**
 * Standard Containers defined as own
 */
 /************************************************************************/
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>

 /************************************************************************/
 /*
  * C++ Stream Stuff
  */
 /************************************************************************/
#include <fstream>
#include <iostream>
#include <iomanip>


#include <locale>
#include <codecvt>

#include <filesystem>

#include <optional>
#include <variant>
#include <bitset>

#include <algorithm>
#include <chrono>
#include <atomic>

#include <any>

#include <functional>

#include <initializer_list>


#include "PlatformDefines.h"
#include "PlatformTypes.h"

/*****************************************************************************/
/**
 * Windows specifics
 */
/*****************************************************************************/
#if USING(SFMX_PLATFORM_WIN32)
  //Undefine min & max
# undef min
# undef max

# if !defined(NOMINMAX) && defined(_MSC_VER)
#   define NOMINMAX     //Required to stop windows.h messing up std::min
# endif
# if defined( __MINGW32__ )
#   include <unistd.h>
# endif
#endif //#if USING(SFMX_PLATFORM_WIN32)


namespace sfmx
{
using std::min;
using std::forward;
using std::ios;

/**
 *   Std alias allocator.
 **/
template<typename T>
using Alloc = std::allocator<T>;

/**
*  @brief Fixed-size array sequence container class, holds its elements in a strict linear
*         sequence
*/
template<typename T, size_t size>
using Array = std::array<T, size>;

/*
 *   Vector wrapper to use along the engine.
 **/
template <typename T, class A = Alloc<T>>
using Vector = std::vector<T, A>;

/**
 * @brief An associative container containing an ordered set of elements.
 */
template<typename T, typename P = std::less<T>, typename A = Alloc<T>>
using Set = std::set<T, P, A>;

template<typename K>
using Hash = std::hash<K>;

/**
 * @brief An associative container that contains key-value pairs with unique keys.
 *        Search, insertion, and removal have average constant-time complexity.
 *
 * @tparam K Type of the keys. Must be unique.
 * @tparam T Type of the values.
 * @tparam Hash Optional hash function; by default, a specialisation of Hash.
 * @tparam Eq Optional function for equality comparison; by default, operator==.
 * @tparam A Optional allocator object for defining storage allocation model; by default, std::allocator.
 */
template<typename K,
  typename T,
  typename Hash = Hash<K>,
  typename Eq = std::equal_to<K>,
  typename A = std::allocator<std::pair<const K, T>>>
using UnorderedMap = std::unordered_map<K, T, Hash, Eq, A>;

/**
 * @brief An associative container that contains a set of unique objects of type Key.
 *        Search, insertion, and removal operations have average constant-time complexity.
 *
 * @tparam K Type of the keys. Must be unique.
 * @tparam Hash Optional hash function; by default, a specialisation of Hash.
 * @tparam Eq Optional function for equality comparison; by default, operator==.
 * @tparam A Optional allocator object for defining storage allocation model; by default, std::allocator.
 */
template<typename K,
  typename Hash = Hash<K>,
  typename Eq = std::equal_to<K>,
  typename A = std::allocator<K>>
  using UnorderedSet = std::unordered_set<K, Hash, Eq, A>;


/**
 * @brief An associative container containing an ordered set of key-value
 *        pairs.
 */
template<typename K,
         typename T,
         typename Compare = std::less<K>,
         typename A = std::allocator<std::pair<const K, T>>>
using Map = std::map<K, T, Compare, A>;

/**
 *   Queue wrapper.
 **/
template< typename T, class Container = std::deque<T>>
using Queue =  std::queue<T, Container>;

/**
 *    Deque wrapper.
 **/
template<typename T, class A = Alloc<T>>
using Deque = std::deque<T, A>;


template<typename T>
using Optional = std::optional<T>;

constexpr auto NullOpt = std::nullopt;

/**
 * @brief Wrapper for the C++ std::variant.
 */
template<typename... Types>
using Variant = std::variant<Types...>;

/**
 * @brief Wrapper for the C++ std::bitset.
 */
template<size_t N>
using BitSet = std::bitset<N>;

/**
 * @brief Wrapper for the C++ std::atomic.
 */
template<typename T>
using Atomic = std::atomic<T>;


/**
 * @brief Wrapper for the C++ std::pair
*/
template<typename T1, typename T2>
using Pair = std::pair<T1, T2>;

/************************************************************************/
/*
 * C++ std::any
 */
/************************************************************************/

#if USING(SFMX_CPP20_OR_LATER)
/**
 * @brief Wrapper for the C++ std::any.
 */
using Any = std::any;

namespace AnyUtils {
  template<typename T>
  concept AnyCompatible = requires(const Any& any) { std::any_cast<T>(any); };
  
  template<AnyCompatible T>
  FORCEINLINE bool
  hasType(const Any& any) noexcept { return any.type() == typeid(T); }
  
  template <AnyCompatible T>
  FORCEINLINE bool
  tryGetValue(const Any& any, T& output) noexcept {
    if (!hasType<T>(any)) {
      return false;
    }
  
    try {
      output = std::any_cast<T>(any);
      return true;
    } catch (const std::bad_any_cast&) {
      return false;
    }
  }
} // namespace AnyUtils
#endif // #ifdef USING(SFMX_CPP20_OR_LATER)


/******************************************************************************************* */
/* Function std*/
/******************************************************************************************* */
/**
 * @brief Function wrapper for std::function.
 * This is a wrapper for std::function that allows
 * for better type inference and usage in the engine.
 * It is used to store functions that can be called with a specific signature.
 */
template<typename Signature>
using Function = std::function<Signature>;

template <typename T>
using InitializerList = std::initializer_list<T>;

using FileSystemPath = std::filesystem::path;
// NOTE: std::filesystem is wrapped by the sfmx::FileSystem class (core/FileSystem.h);
// do not re-add a `namespace FileSystem = std::filesystem` alias here (name clash).

} // namespace chEngineSDK
