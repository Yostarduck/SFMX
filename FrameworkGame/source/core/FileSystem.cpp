#include "core/FileSystem.h"

#include <filesystem>

#include "core/FileDataStream.h"

namespace sfmx
{

namespace fs = std::filesystem;

SPtr<DataStream>
FileSystem::openFile(const FileSystemPath& path, AccessModeFlags mode) {
  auto stream = MakeShared<FileDataStream>(path, mode);
  if (!stream->isOpen()) {
    return nullptr;
  }
  return stream;
}

SPtr<DataStream>
FileSystem::createAndOpenFile(const FileSystemPath& path) {
  if (path.has_parent_path()) {
    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);
  }
  return openFile(path, AccessMode::kWrite);
}

bool
FileSystem::dumpMemStreamIntoFile(const SPtr<DataStream>& source,
                                  const FileSystemPath& path) {
  if (nullptr == source) {
    return false;
  }

  SPtr<DataStream> out = createAndOpenFile(path);
  if (nullptr == out) {
    return false;
  }

  // Backing-agnostic copy: pull from the source through the public interface so
  // this works for memory, file, or any future stream type.
  constexpr size_t kChunk = 64u * 1024u;
  Vector<uint8> buffer(kChunk);

  source->seek(0);
  size_t remaining = source->size();
  while (remaining > 0) {
    const size_t toRead = (remaining < kChunk) ? remaining : kChunk;
    const size_t got = source->read(buffer.data(), toRead);
    if (0 == got) {
      break;
    }
    out->write(buffer.data(), got);
    remaining -= got;
  }

  out->close();
  return true;
}

Vector<uint8>
FileSystem::fastRead(const FileSystemPath& path) {
  Vector<uint8> data;

  SPtr<DataStream> in = openFile(path, AccessMode::kRead);
  if (nullptr == in) {
    return data;
  }

  data.resize(in->size());
  if (!data.empty()) {
    const size_t got = in->read(data.data(), data.size());
    data.resize(got);
  }
  return data;
}

bool
FileSystem::exists(const FileSystemPath& path) {
  std::error_code ec;
  return fs::exists(path, ec);
}

bool
FileSystem::isFile(const FileSystemPath& path) {
  std::error_code ec;
  return fs::is_regular_file(path, ec);
}

bool
FileSystem::isDirectory(const FileSystemPath& path) {
  std::error_code ec;
  return fs::is_directory(path, ec);
}

bool
FileSystem::createDirectories(const FileSystemPath& path) {
  std::error_code ec;
  fs::create_directories(path, ec);
  // create_directories returns false when the dir already existed (no error),
  // so report on the error code, not the return value.
  return !ec;
}

bool
FileSystem::remove(const FileSystemPath& path) {
  std::error_code ec;
  return fs::remove(path, ec) && !ec;
}

bool
FileSystem::removeAll(const FileSystemPath& path) {
  std::error_code ec;
  fs::remove_all(path, ec);
  return !ec;
}

bool
FileSystem::rename(const FileSystemPath& from, const FileSystemPath& to) {
  std::error_code ec;
  fs::rename(from, to, ec);
  return !ec;
}

FileSystemPath
FileSystem::tempDirectory() {
  std::error_code ec;
  return fs::temp_directory_path(ec);
}

void
FileSystem::forEachFileChild(const FileSystemPath& path,
                             const Function<void(const FileSystemPath&)>& fn) {
  std::error_code ec;
  if (!fs::is_directory(path, ec)) {
    return;
  }
  for (const auto& entry : fs::directory_iterator(path, ec)) {
    fn(entry.path());
  }
}

void
FileSystem::forEachFileChildRecursive(const FileSystemPath& path,
                                      const Function<void(const FileSystemPath&)>& fn) {
  std::error_code ec;
  if (!fs::is_directory(path, ec)) {
    return;
  }
  for (const auto& entry : fs::recursive_directory_iterator(path, ec)) {
    fn(entry.path());
  }
}

} // namespace sfmx
