#include <doctest/doctest.h>

#include "core/platform/Prerequisites.h"
#include "core/MemoryDataStream.h"

using namespace sfmx;

namespace {

struct Pod {
  int32 i;
  float f;
};

}  // namespace

TEST_CASE("DataStream POD + string round-trip") {
  MemoryDataStream s;
  const uint32 a = 0xDEADBEEFu;
  const float b = 3.5f;
  const Pod pod{-7, 1.25f};

  s << a << b << pod;
  s.writeString("hello sfmx");
  s.writeString("");

  CHECK(s.size() == sizeof(a) + sizeof(b) + sizeof(pod)
                     + sizeof(uint64) + 10
                     + sizeof(uint64));

  s.seek(0);
  uint32 ra = 0;
  float rb = 0.0f;
  Pod rpod{};
  s >> ra >> rb >> rpod;
  const String rs = s.readString();
  const String rsEmpty = s.readString();

  CHECK(ra == a);
  CHECK(rb == b);
  CHECK(rpod.i == pod.i);
  CHECK(rpod.f == pod.f);
  CHECK(rs == "hello sfmx");
  CHECK(rsEmpty.empty());
  CHECK(s.isAtEnd());
}

TEST_CASE("DataStream seek / skip") {
  MemoryDataStream s;
  const uint32 a = 0xDEADBEEFu;
  const float b = 3.5f;
  s << a << b;

  s.seek(0);
  s.skip(sizeof(uint32));
  float onlyB = 0.0f;
  s >> onlyB;
  CHECK(onlyB == b);
  CHECK(s.tell() == sizeof(uint32) + sizeof(float));
}

TEST_CASE("DataStream growable write") {
  MemoryDataStream g;
  Vector<uint8> blob(1000, 0xABu);
  g.write(blob.data(), blob.size());
  CHECK(g.size() == 1000);
  g.seek(0);
  Vector<uint8> readback(1000, 0);
  const size_t got = g.read(readback.data(), readback.size());
  CHECK(got == 1000);
  CHECK(readback == blob);
}

TEST_CASE("DataStream clone independence") {
  MemoryDataStream s;
  const uint32 a = 0xDEADBEEFu;
  s << a;

  s.seek(0);
  SPtr<DataStream> c = s.clone();
  DataStream& cref = *c;
  uint32 ca = 0;
  cref >> ca;
  CHECK(ca == a);

  const uint32 marker = 0x12345678u;
  cref.seek(0);
  cref << marker;
  s.seek(0);
  uint32 origA = 0;
  s >> origA;
  CHECK(origA == a);
}

TEST_CASE("DataStream read past end") {
  MemoryDataStream s;
  const uint32 a = 0xDEADBEEFu;
  s << a;
  s.seek(s.size());
  uint32 nothing = 0;
  CHECK(s.read(&nothing, sizeof(nothing)) == 0);
}
