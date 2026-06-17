#include <doctest/doctest.h>

#include <nanobench/nanobench.h>

#include "core/platform/Prerequisites.h"

namespace sfmx {
using Benchmark = ankerl::nanobench::Bench;

// Making Catch2-like Benchmark tests
#define DONOTOPTIMIZE(...) ankerl::nanobench::doNotOptimizeAway(__VA_ARGS__);

#define BENCHMARK(name, ...) ankerl::nanobench::Bench().run(name, __VA_ARGS__)

#define BENCHMARKEPOCHS(name, times, ...) \
ankerl::nanobench::Bench().epochs(times).run(name, __VA_ARGS__)
}