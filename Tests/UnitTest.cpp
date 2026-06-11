// SFMX test runner. This is the entry point; each test group lives in its own
// .cpp (FlagTest.cpp, ...) and is invoked here. Add a new group by including
// its header and calling its run function before report().

#include "TestRunner.h"

#include "FlagTest.h"
#include "DataStreamTest.h"
#include "FileSystemTest.h"

int
main() {
  runFlagTests();
  runDataStreamTests();
  runFileSystemTests();
  // runFooTests();   // future test groups go here

  return sfmxtest::report();
}
