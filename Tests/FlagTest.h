#pragma once

// Runs the Flags<> test group (uses SFMX_CHECK from TestRunner.h for runtime
// checks; most coverage is compile-time static_assert inside FlagTest.cpp, so
// the file failing to compile is itself a failure).
void
runFlagTests();
