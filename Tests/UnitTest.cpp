// SFMX test runner. This is the entry point; each test group lives in its own
// .cpp (FlagTest.cpp, DataStreamTest.cpp, ...) and is discovered automatically
// via doctest TEST_CASE macros.

#define DOCTEST_CONFIG_IMPLEMENT
#define ANKERL_NANOBENCH_IMPLEMENT

#include "utils/UnitTest.h"


int
main(int argc, char* argv[]) {

  doctest::Context context;
  context.applyCommandLine(argc, argv);

  int res = context.run();
  if (context.shouldExit()) {
    return res;
  }

  context.clearFilters();
  return res + EXIT_SUCCESS;
}
