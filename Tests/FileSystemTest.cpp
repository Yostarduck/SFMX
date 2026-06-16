#include <doctest/doctest.h>

#include "core/platform/Prerequisites.h"
#include "core/FileSystem.h"
#include "core/MemoryDataStream.h"

using namespace sfmx;

namespace {

FileSystemPath
makeTempDir() {
  auto d = FileSystem::tempDirectory() / "sfmx_fs_test";
  FileSystem::removeAll(d);
  return d;
}

}  // namespace

TEST_CASE("FileSystem write + read round-trip") {
  const auto dir = makeTempDir();
  const auto file = dir / "nested" / "blob.bin";

  auto mem = MakeShared<MemoryDataStream>();
  const uint32 magic = 0x53464D58u;
  const float version = 0.2f;
  (*mem) << magic << version;
  mem->writeString("payload");
  const Vector<uint8> tail(500, 0xCDu);
  mem->write(tail.data(), tail.size());
  const size_t expectedSize = mem->size();

  CHECK(FileSystem::dumpMemStreamIntoFile(mem, file));
  CHECK(FileSystem::isFile(file));
  CHECK(FileSystem::isDirectory(dir));

  const Vector<uint8> raw = FileSystem::fastRead(file);
  CHECK(raw.size() == expectedSize);

  SPtr<DataStream> in = FileSystem::openFile(file, AccessMode::kRead);
  CHECK(nullptr != in);
  if (nullptr != in) {
    CHECK(in->size() == expectedSize);

    uint32 rMagic = 0;
    float rVersion = 0.0f;
    (*in) >> rMagic >> rVersion;
    const String rStr = in->readString();

    CHECK(rMagic == magic);
    CHECK(rVersion == version);
    CHECK(rStr == "payload");
    CHECK(in->tell() == sizeof(magic) + sizeof(version) + sizeof(uint64) + 7);
    CHECK(!in->isAtEnd());
  }
  in.reset();

  FileSystem::removeAll(dir);
}

TEST_CASE("FileSystem pre-buffer into memory") {
  const auto dir = makeTempDir();
  const auto file = dir / "nested" / "blob.bin";

  auto mem = MakeShared<MemoryDataStream>();
  const uint32 magic = 0x53464D58u;
  (*mem) << magic;
  FileSystem::dumpMemStreamIntoFile(mem, file);

  SPtr<DataStream> in = FileSystem::openFile(file, AccessMode::kRead);
  if (nullptr != in) {
    MemoryDataStream buffered(in);
    CHECK(buffered.size() == mem->size());
    uint32 bMagic = 0;
    buffered >> bMagic;
    CHECK(bMagic == magic);
  }

  FileSystem::removeAll(dir);
}

TEST_CASE("FileSystem open missing file returns null") {
  const auto dir = makeTempDir();
  CHECK(nullptr == FileSystem::openFile(dir / "does_not_exist.bin"));
  FileSystem::removeAll(dir);
}

TEST_CASE("FileSystem write-stream cursor and size tracking") {
  const auto dir = makeTempDir();
  FileSystem::createDirectories(dir);
  const auto wfile = dir / "write.bin";

  {
    SPtr<DataStream> w = FileSystem::openFile(wfile, AccessMode::kWrite);
    CHECK(nullptr != w);
    if (nullptr != w) {
      const Vector<uint8> hundred(100, 0x11u);
      w->write(hundred.data(), hundred.size());
      CHECK(w->size() == 100);
      CHECK(w->tell() == 100);

      w->seek(50);
      CHECK(w->tell() == 50);
      const Vector<uint8> ten(10, 0x22u);
      w->write(ten.data(), ten.size());
      CHECK(w->tell() == 60);
      CHECK(w->size() == 100);

      w->seek(200);
      w->write(ten.data(), ten.size());
      CHECK(w->tell() == 210);
      CHECK(w->size() == 210);

      w->seek(10);
      w->skip(5);
      CHECK(w->tell() == 15);
      w->skip(-100);
      CHECK(w->tell() == 0);
    }
    w.reset();
    CHECK(FileSystem::fastRead(wfile).size() == 210);
  }

  FileSystem::removeAll(dir);
}

TEST_CASE("FileSystem read+write in-place patch") {
  const auto dir = makeTempDir();
  FileSystem::createDirectories(dir);
  const auto pfile = dir / "patch.bin";

  {
    SPtr<DataStream> sw = FileSystem::openFile(pfile, AccessMode::kWrite);
    CHECK(nullptr != sw);
    if (nullptr != sw) {
      Vector<uint8> seed(256);
      for (size_t i = 0; i < seed.size(); ++i)
        seed[i] = static_cast<uint8>(i);
      sw->write(seed.data(), seed.size());
    }
    sw.reset();

    SPtr<DataStream> rw =
        FileSystem::openFile(pfile, AccessMode::kRead | AccessMode::kWrite);
    CHECK(nullptr != rw);
    if (nullptr != rw) {
      CHECK(rw->size() == 256);

      uint8 head[4] = {};
      rw->read(head, 4);
      CHECK(head[0] == 0);
      CHECK(head[1] == 1);
      CHECK(head[2] == 2);
      CHECK(head[3] == 3);

      const uint8 patch[4] = {0xA0, 0xA1, 0xA2, 0xA3};
      rw->write(patch, 4);

      uint8 mid[4] = {};
      rw->read(mid, 4);
      CHECK(mid[0] == 8);
      CHECK(mid[1] == 9);
      CHECK(mid[2] == 10);
      CHECK(mid[3] == 11);

      CHECK(rw->size() == 256);

      rw->seek(4);
      uint8 back[4] = {};
      rw->read(back, 4);
      CHECK(back[0] == 0xA0);
      CHECK(back[1] == 0xA1);
      CHECK(back[2] == 0xA2);
      CHECK(back[3] == 0xA3);
    }
    rw.reset();

    const Vector<uint8> after = FileSystem::fastRead(pfile);
    CHECK(after.size() == 256);
    if (after.size() == 256) {
      CHECK(after[4] == 0xA0);
      CHECK(after[5] == 0xA1);
      CHECK(after[6] == 0xA2);
      CHECK(after[7] == 0xA3);
      for (size_t i = 0; i < 256; ++i) {
        if (i >= 4 && i <= 7) continue;
        CHECK(after[i] == static_cast<uint8>(i));
      }
    }

    CHECK(nullptr == FileSystem::openFile(
        dir / "nope_rw.bin", AccessMode::kRead | AccessMode::kWrite));

    SPtr<DataStream> rwt = FileSystem::openFile(
        dir / "rwtrunc.bin",
        AccessMode::kRead | AccessMode::kWrite | AccessMode::kTruncate);
    CHECK(nullptr != rwt);
    if (nullptr != rwt) {
      CHECK(rwt->size() == 0);
      const uint32 v = 0xCAFEBABEu;
      (*rwt) << v;
      rwt->seek(0);
      uint32 r = 0;
      (*rwt) >> r;
      CHECK(r == v);
      CHECK(rwt->size() == sizeof(v));
    }
    rwt.reset();
  }

  FileSystem::removeAll(dir);
}
