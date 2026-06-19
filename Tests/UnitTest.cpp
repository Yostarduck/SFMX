// SFMX test runner. This is the entry point; each test group lives in its own
// .cpp (FlagTest.cpp, DataStreamTest.cpp, ...) and is discovered automatically
// via doctest TEST_CASE macros.

#define DOCTEST_CONFIG_IMPLEMENT
#define ANKERL_NANOBENCH_IMPLEMENT
#include "utils/UnitTest.h"
#include "utils/MemoryPoolHandler.h"

using namespace sfmx;


int
main(int argc, char* argv[]) {

  doctest::Context context;
  context.applyCommandLine(argc, argv);

  int res = context.run();
  if (context.shouldExit()) {
    return res;
  }

  context.clearFilters();

  // Don't shut down MemoryPoolHandler here — static destruction that follows
  // may trigger component/SceneNode destructors that still reference pools.
  // reset() destroys remaining elements while keeping pools registered so any
  // late cleanup during static teardown can still deallocate safely.
  if (MemoryPoolHandler::isStarted()) {
    MemoryPoolHandler::instance().reset();
  }

  return res + EXIT_SUCCESS;
}
