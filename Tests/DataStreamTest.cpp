// DataStream / MemoryDataStream test group: POD and string round-trips, the
// growable write path, seek/skip, and clone independence.

#include "DataStreamTest.h"

#include <iostream>

#include "core/platform/Prerequisites.h"
#include "core/MemoryDataStream.h"
#include "core/DataStreamTypes.h"  // UUID / Vector serialization overloads

#include "TestRunner.h"

using namespace sfmx;

namespace {

struct Pod {
  int32 i;
  float f;
};

}  // namespace

// ---------------------------------------------------------------------------
void
runDataStreamTests() {
  const int failedBefore = sfmxtest::g_checksFailed;

  // -- POD + string round-trip on a growable memory stream ------------------
  MemoryDataStream s;  // empty, read+write, grows on write
  const uint32 a = 0xDEADBEEFu;
  const float b = 3.5f;
  const Pod pod{ -7, 1.25f };

  s << a << b << pod;
  s.writeString("hello sfmx");
  s.writeString("");  // empty string must round-trip too

  SFMX_CHECK(s.size() == sizeof(a) + sizeof(b) + sizeof(pod)
                         + sizeof(uint64) + 10   // "hello sfmx"
                         + sizeof(uint64));      // ""

  s.seek(0);
  uint32 ra = 0;
  float rb = 0.0f;
  Pod rpod{};
  s >> ra >> rb >> rpod;
  const String rs = s.readString();
  const String rsEmpty = s.readString();

  SFMX_CHECK(ra == a);
  SFMX_CHECK(rb == b);
  SFMX_CHECK(rpod.i == pod.i && rpod.f == pod.f);
  SFMX_CHECK(rs == "hello sfmx");
  SFMX_CHECK(rsEmpty.empty());
  SFMX_CHECK(s.isAtEnd());

  // -- seek / skip ----------------------------------------------------------
  s.seek(0);
  s.skip(sizeof(uint32));      // hop over 'a'
  float onlyB = 0.0f;
  s >> onlyB;
  SFMX_CHECK(onlyB == b);
  SFMX_CHECK(s.tell() == sizeof(uint32) + sizeof(float));

  // -- growable: write more than any initial capacity -----------------------
  MemoryDataStream g;
  Vector<uint8> blob(1000, 0xABu);
  g.write(blob.data(), blob.size());
  SFMX_CHECK(g.size() == 1000);
  g.seek(0);
  Vector<uint8> readback(1000, 0);
  const size_t got = g.read(readback.data(), readback.size());
  SFMX_CHECK(got == 1000);
  SFMX_CHECK(readback == blob);

  // -- clone is an independent copy -----------------------------------------
  s.seek(0);
  SPtr<DataStream> c = s.clone();
  DataStream& cref = *c;
  uint32 ca = 0;
  cref >> ca;
  SFMX_CHECK(ca == a);
  // Writing into the clone must not affect the original.
  const uint32 marker = 0x12345678u;
  cref.seek(0);
  cref << marker;
  s.seek(0);
  uint32 origA = 0;
  s >> origA;
  SFMX_CHECK(origA == a);

  // -- read past end returns a short count ----------------------------------
  s.seek(s.size());
  uint32 nothing = 0;
  SFMX_CHECK(s.read(&nothing, sizeof(nothing)) == 0);

  // -- Vector<T> (trivially-copyable) round-trip ----------------------------
  MemoryDataStream vs;
  const Vector<uint32> nums = {1u, 2u, 3u, 0xFFFFFFFFu, 42u};
  const Vector<uint32> empty;
  vs << nums << empty;

  vs.seek(0);
  Vector<uint32> rNums;
  Vector<uint32> rEmpty;
  vs >> rNums >> rEmpty;
  SFMX_CHECK(rNums == nums);
  SFMX_CHECK(rEmpty.empty());
  SFMX_CHECK(vs.isAtEnd());
  // Layout: [u64 count][5*u32] + [u64 count]
  SFMX_CHECK(vs.size() == sizeof(uint64) + 5 * sizeof(uint32) + sizeof(uint64));

  // -- UUID round-trips as 16 raw bytes -------------------------------------
  // Qualify sfmx::UUID: on Windows <rpcdce.h> (pulled in by stduuid's system
  // generator) defines a global ::UUID, so bare UUID is ambiguous here.
  MemoryDataStream us;
  const sfmx::UUID id = sfmx::UUID::createRandom();
  const sfmx::UUID named = sfmx::UUID::createFromName("ParticleSystemComponent");
  us << id << named;
  SFMX_CHECK(us.size() == 32);  // 16 + 16, no string overhead

  us.seek(0);
  sfmx::UUID rId;
  sfmx::UUID rNamed;
  us >> rId >> rNamed;
  SFMX_CHECK(rId == id);
  SFMX_CHECK(rNamed == named);

  if (sfmxtest::g_checksFailed == failedBefore) {
    std::cout << "[DataStreamTest] passed\n";
  }
}
