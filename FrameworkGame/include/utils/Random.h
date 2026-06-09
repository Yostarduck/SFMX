#pragma once

#include <chrono>
#include <random>

#include "core/platform/Prerequisites.h"

namespace sfmx {

class SFMX_UTILITY_EXPORT Random {

 public:

  static void seed() {
    if (!m_init)
      init();
    m_mt.seed(std::chrono::system_clock::now().time_since_epoch().count());
  }

  static void seed(int num) {
    if (!m_init)
      init();
    m_mt.seed(num);
  }

  template<typename T>
  NODISCARD static T get() {
    if (!m_init)
      init();
    return static_cast<T>(m_mt());
  }

  template<typename T>
  NODISCARD static T range(T inclusiveStart, T exclusiveEnd) {
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

  template<typename T>
  NODISCARD static T weighted(const Vector<float>& weights) {
    if (!m_init)
      init();

    std::discrete_distribution<size_t> dist(weights.begin(), weights.end());
    return static_cast<T>(dist(m_mt));
  }

  NODISCARD static int32 diceThrow(int32 amount, int32 diceFace) {
    int32 sum = 0;
    for (int32 i = 0; i < amount; ++i) {
      sum += Random::range<int32>(1, diceFace + 1);
    }
    return sum;
  }

  NODISCARD static bool isInit() { return m_init; }

 private:

  static void init() {
    m_mt.seed(std::random_device{}());
    m_init = true;
  }

  static std::mt19937 m_mt;
  static bool m_init;
};

} // namespace sfmx
