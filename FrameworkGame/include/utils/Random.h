/************************************************************************/
/**
 * @file Random.h
 * @author Swampertor
 * @date 2026/06/9
 * @brief  Class with random number generation functions.
 */
/************************************************************************/
#pragma once

#include "core/platform/Prerequisites.h"

#include <chrono>
#include <random>

namespace sfmx {

/**
 * @brief A Random utilities class that generates random
 *        numbers, via a static mt1997 variable
 * 
 */
class SFMX_UTILITY_EXPORT Random {

 public:

 /** @brief Sets a new seed based on the current time epoch */
  static void 
  seed() {
    if (!m_init)
      init();
    m_mt.seed(
      std::chrono::system_clock::now().time_since_epoch().count());
  }

  /** @brief Sets a specific seed from a user defined number */
  static void seed(int num) {
    if (!m_init)
      init();
    m_mt.seed(num);
  }

  /** @brief Gets a random number casted as T value */
  template<typename T>
  NODISCARD static T 
  get() {
    if (!m_init)
      init();
    return static_cast<T>(m_mt());
  }

  /** @brief Gets a T value in a [inclusive, exclusive) range */
  template<typename T>
  NODISCARD static T 
  range(T inclusiveStart, T exclusiveEnd) {
    if (!m_init)
      init();

    if constexpr (std::is_integral_v<T>) {
      // uniform_int_distribution is [a, b] inclusive; adjust upper bound.
      std::uniform_int_distribution<T> dist(inclusiveStart,
                                            exclusiveEnd - 1);
      return dist(m_mt);
    } else {
      std::uniform_real_distribution<T> dist(inclusiveStart, exclusiveEnd);
      return dist(m_mt);
    }
  }

  /** @brief based on a list of weights, generate a T number */
  template<typename T>
  NODISCARD static T weighted(const Vector<float>& weights) {
    if (!m_init)
      init();

    std::discrete_distribution<size_t> dist(weights.begin(), weights.end());
    return static_cast<T>(dist(m_mt));
  }

  /** @brief Generates a random value based on a probability curve of n-faced dice */
  NODISCARD static int32 diceThrow(int32 amount, int32 diceFace) {
    int32 sum = 0;
    for (int32 i = 0; i < amount; ++i) {
      sum += Random::range<int32>(1, diceFace + 1);
    }
    return sum;
  }

  /** @brief checks if the internal random generator has ben set already */
  NODISCARD static bool isInit() { return m_init; }

 private:

  /** @brief Internally initializes the mt19937 variable */
  static void init() {
    m_mt.seed(std::random_device{}());
    m_init = true;
  }

  static std::mt19937 m_mt;
  static bool m_init;
};

} // namespace sfmx
