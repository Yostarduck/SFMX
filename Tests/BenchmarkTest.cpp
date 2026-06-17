#include "utils/UnitTest.h"

#include "core/MemoryDataStream.h"

using namespace sfmx;

TEST_CASE("MemoryDataStream write throughput") {
  BENCHMARK("MemoryDataStream sequential write", [&]() {
    MemoryDataStream stream;
    for (int32 i = 0; i < 10'000; ++i) {
      stream << i;
    }
    DONOTOPTIMIZE(stream);
  });
}

TEST_CASE("MemoryDataStream read throughput") {
  MemoryDataStream stream;
  for (int32 i = 0; i < 10'000; ++i) {
    stream << i;
  }
  stream.seek(0);

  BENCHMARK("MemoryDataStream sequential read", [&]() {
    int32 sum = 0;
    int32 val = 0;
    stream.seek(0);
    while (stream.tell() < stream.size()) {
      stream >> val;
      sum += val;
    }
    DONOTOPTIMIZE(sum);
  });
}
