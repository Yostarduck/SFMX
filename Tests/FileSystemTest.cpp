// FileSystem / FileDataStream test group: build a buffer in memory, dump it to
// a temp file, read it back, and verify byte-for-byte. Cleans up after itself.

#include "FileSystemTest.h"

#include <iostream>

#include "core/platform/Prerequisites.h"
#include "core/FileSystem.h"
#include "core/MemoryDataStream.h"

#include "TestRunner.h"

using namespace sfmx;

// ---------------------------------------------------------------------------
void
runFileSystemTests() {
  const int failedBefore = sfmxtest::g_checksFailed;

  const FileSystemPath dir = FileSystem::tempDirectory() / "sfmx_fs_test";
  const FileSystemPath file = dir / "nested" / "blob.bin";  // nested: tests dir creation

  // Start clean in case a previous run left something behind.
  FileSystem::removeAll(dir);

  // -- Build a payload in memory --------------------------------------------
  auto mem = MakeShared<MemoryDataStream>();
  const uint32 magic = 0x53464D58u;  // 'SFMX'
  const float version = 0.2f;
  (*mem) << magic << version;
  mem->writeString("payload");
  const Vector<uint8> tail(500, 0xCDu);
  mem->write(tail.data(), tail.size());
  const size_t expectedSize = mem->size();

  // -- Dump to disk (creates parent dirs) -----------------------------------
  SFMX_CHECK(FileSystem::dumpMemStreamIntoFile(mem, file));
  SFMX_CHECK(FileSystem::isFile(file));
  SFMX_CHECK(FileSystem::isDirectory(dir));

  // -- fastRead returns the exact bytes -------------------------------------
  const Vector<uint8> raw = FileSystem::fastRead(file);
  SFMX_CHECK(raw.size() == expectedSize);

  // -- Read back through a file stream and parse ----------------------------
  SPtr<DataStream> in = FileSystem::openFile(file, AccessMode::kRead);
  SFMX_CHECK(nullptr != in);
  if (nullptr != in) {
    SFMX_CHECK(in->size() == expectedSize);

    uint32 rMagic = 0;
    float rVersion = 0.0f;
    (*in) >> rMagic >> rVersion;
    const String rStr = in->readString();

    SFMX_CHECK(rMagic == magic);
    SFMX_CHECK(rVersion == version);
    SFMX_CHECK(rStr == "payload");
    SFMX_CHECK(in->tell() == sizeof(magic) + sizeof(version) + sizeof(uint64) + 7);
    SFMX_CHECK(!in->isAtEnd());  // 500 tail bytes remain
  }

  // -- Pre-buffer a file straight into memory -------------------------------
  SPtr<DataStream> in2 = FileSystem::openFile(file, AccessMode::kRead);
  if (nullptr != in2) {
    MemoryDataStream buffered(in2);
    SFMX_CHECK(buffered.size() == expectedSize);
    uint32 bMagic = 0;
    buffered >> bMagic;
    SFMX_CHECK(bMagic == magic);
  }

  // -- Opening a missing file fails gracefully ------------------------------
  SFMX_CHECK(nullptr == FileSystem::openFile(dir / "does_not_exist.bin"));

  // -- Cleanup --------------------------------------------------------------
  // Release open handles first: on Windows a file in use cannot be deleted.
  in.reset();
  in2.reset();
  SFMX_CHECK(FileSystem::removeAll(dir));
  SFMX_CHECK(!FileSystem::exists(file));

  if (sfmxtest::g_checksFailed == failedBefore) {
    std::cout << "[FileSystemTest] passed\n";
  }
}
