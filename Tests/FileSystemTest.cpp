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

  // -- Write-stream cursor / size tracking (seek + write) -------------------
  const FileSystemPath wfile = dir / "write.bin";
  {
    SPtr<DataStream> w = FileSystem::openFile(wfile, AccessMode::kWrite);
    SFMX_CHECK(nullptr != w);
    if (nullptr != w) {
      const Vector<uint8> hundred(100, 0x11u);
      w->write(hundred.data(), hundred.size());
      SFMX_CHECK(w->size() == 100);
      SFMX_CHECK(w->tell() == 100);

      // Overwrite within bounds: size must NOT grow.
      w->seek(50);
      SFMX_CHECK(w->tell() == 50);
      const Vector<uint8> ten(10, 0x22u);
      w->write(ten.data(), ten.size());
      SFMX_CHECK(w->tell() == 60);
      SFMX_CHECK(w->size() == 100);

      // Write past the current end: size grows to the new cursor.
      w->seek(200);
      w->write(ten.data(), ten.size());
      SFMX_CHECK(w->tell() == 210);
      SFMX_CHECK(w->size() == 210);

      // skip is relative to our own cursor and clamps at 0.
      w->seek(10);
      w->skip(5);
      SFMX_CHECK(w->tell() == 15);
      w->skip(-100);
      SFMX_CHECK(w->tell() == 0);
    }
    w.reset();
    SFMX_CHECK(FileSystem::fastRead(wfile).size() == 210);  // on-disk size matches
  }

  // -- Read+write in place: edit a file without buffering it whole ----------
  const FileSystemPath pfile = dir / "patch.bin";
  {
    // Seed a 256-byte file where byte i == i.
    SPtr<DataStream> sw = FileSystem::openFile(pfile, AccessMode::kWrite);
    SFMX_CHECK(nullptr != sw);
    if (nullptr != sw) {
      Vector<uint8> seed(256);
      for (size_t i = 0; i < seed.size(); ++i) {
        seed[i] = static_cast<uint8>(i);
      }
      sw->write(seed.data(), seed.size());
    }
    sw.reset();

    // Open in place (in|out, content preserved) and patch the middle.
    SPtr<DataStream> rw =
        FileSystem::openFile(pfile, AccessMode::kRead | AccessMode::kWrite);
    SFMX_CHECK(nullptr != rw);
    if (nullptr != rw) {
      SFMX_CHECK(rw->size() == 256);

      uint8 head[4] = {};
      rw->read(head, 4);                       // read 0..3
      SFMX_CHECK(head[0] == 0 && head[1] == 1 && head[2] == 2 && head[3] == 3);

      const uint8 patch[4] = {0xA0, 0xA1, 0xA2, 0xA3};
      rw->write(patch, 4);                     // read -> write switch; patch 4..7

      uint8 mid[4] = {};
      rw->read(mid, 4);                        // write -> read switch; read 8..11
      SFMX_CHECK(mid[0] == 8 && mid[1] == 9 && mid[2] == 10 && mid[3] == 11);

      SFMX_CHECK(rw->size() == 256);           // in-bounds patch didn't grow

      rw->seek(4);
      uint8 back[4] = {};
      rw->read(back, 4);
      SFMX_CHECK(back[0] == 0xA0 && back[1] == 0xA1 &&
                 back[2] == 0xA2 && back[3] == 0xA3);
    }
    rw.reset();

    // Reopen read-only: patch persisted, the rest intact, size unchanged.
    const Vector<uint8> after = FileSystem::fastRead(pfile);
    SFMX_CHECK(after.size() == 256);
    if (after.size() == 256) {
      bool ok = after[4] == 0xA0 && after[5] == 0xA1 &&
                after[6] == 0xA2 && after[7] == 0xA3;
      for (size_t i = 0; i < 256 && ok; ++i) {
        if (i >= 4 && i <= 7) {
          continue;
        }
        ok = (after[i] == static_cast<uint8>(i));
      }
      SFMX_CHECK(ok);
    }

    // in|out (no kTruncate) requires the file to already exist.
    SFMX_CHECK(nullptr == FileSystem::openFile(
        dir / "nope_rw.bin", AccessMode::kRead | AccessMode::kWrite));

    // kTruncate gives a fresh, empty read+write file.
    SPtr<DataStream> rwt = FileSystem::openFile(
        dir / "rwtrunc.bin",
        AccessMode::kRead | AccessMode::kWrite | AccessMode::kTruncate);
    SFMX_CHECK(nullptr != rwt);
    if (nullptr != rwt) {
      SFMX_CHECK(rwt->size() == 0);
      const uint32 v = 0xCAFEBABEu;
      (*rwt) << v;
      rwt->seek(0);
      uint32 r = 0;
      (*rwt) >> r;
      SFMX_CHECK(r == v);
      SFMX_CHECK(rwt->size() == sizeof(v));
    }
    rwt.reset();
  }

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
