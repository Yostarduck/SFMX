#pragma once

// Minimal shared test utilities (the repo has no test framework yet).
// - SFMX_CHECK(cond): records a runtime assertion; logs file:line on failure.
// - report(): prints the run summary and yields a process exit code.
// Counters are inline (C++17) so all test .cpp files share one instance.

#include <iostream>

namespace sfmxtest
{

inline int g_checksRun = 0;
inline int g_checksFailed = 0;

// Returns 0 if every check passed, 1 otherwise. Use as main()'s return value.
inline int
report() {
  if (0 == g_checksFailed) {
    std::cout << "[UnitTest] All " << g_checksRun << " checks passed.\n";
    return 0;
  }
  std::cerr << "[UnitTest] " << g_checksFailed << " of " << g_checksRun
            << " checks FAILED.\n";
  return 1;
}

} // namespace sfmxtest

#define SFMX_CHECK(cond)                                                        \
  do {                                                                          \
    ++::sfmxtest::g_checksRun;                                                  \
    if (!(cond)) {                                                              \
      ++::sfmxtest::g_checksFailed;                                             \
      std::cerr << "[CHECK FAIL] " << __FILE__ << ":" << __LINE__ << "  "       \
                << #cond << "\n";                                               \
    }                                                                           \
  } while (0)
