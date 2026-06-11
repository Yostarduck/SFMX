#include "core/FileDataStream.h"

namespace sfmx
{

FileDataStream::FileDataStream(const FileSystemPath& path, AccessModeFlags mode)
  : DataStream(mode),
    m_path(path) {
  std::ios::openmode openMode = std::ios::binary;
  if (isWriteable()) {
    openMode |= std::ios::out | std::ios::trunc;
  }
  else {
    openMode |= std::ios::in;
  }

  m_stream.open(m_path, openMode);
  if (!m_stream.is_open()) {
    return;
  }

  if (isWriteable()) {
    m_size = 0;
  }
  else {
    // Measure the file, then rewind so reading starts at the beginning.
    m_stream.seekg(0, std::ios::end);
    m_size = static_cast<size_t>(m_stream.tellg());
    m_stream.seekg(0, std::ios::beg);
  }
}

FileDataStream::~FileDataStream() {
  close();
}

size_t
FileDataStream::read(void* dst, size_t bytes) {
  if (!isReadable() || !m_stream.is_open()) {
    return 0;
  }
  m_stream.read(static_cast<char*>(dst), static_cast<std::streamsize>(bytes));
  return static_cast<size_t>(m_stream.gcount());
}

size_t
FileDataStream::write(const void* src, size_t bytes) {
  if (!isWriteable() || !m_stream.is_open()) {
    return 0;
  }
  m_stream.write(static_cast<const char*>(src), static_cast<std::streamsize>(bytes));
  if (!m_stream.good()) {
    return 0;
  }
  const size_t end = static_cast<size_t>(m_stream.tellp());
  if (end > m_size) {
    m_size = end;
  }
  return bytes;
}

void
FileDataStream::seek(size_t pos) {
  m_stream.clear();  // drop any eof/fail flag from a prior read
  if (isWriteable()) {
    m_stream.seekp(static_cast<std::streamoff>(pos), std::ios::beg);
  }
  else {
    m_stream.seekg(static_cast<std::streamoff>(pos), std::ios::beg);
  }
}

void
FileDataStream::skip(int64 count) {
  m_stream.clear();
  if (isWriteable()) {
    m_stream.seekp(static_cast<std::streamoff>(count), std::ios::cur);
  }
  else {
    m_stream.seekg(static_cast<std::streamoff>(count), std::ios::cur);
  }
}

size_t
FileDataStream::tell() const {
  m_stream.clear();
  const std::streampos pos = isWriteable() ? m_stream.tellp() : m_stream.tellg();
  return (pos < 0) ? 0 : static_cast<size_t>(pos);
}

bool
FileDataStream::isAtEnd() const {
  return tell() >= m_size;
}

void
FileDataStream::close() {
  if (m_stream.is_open()) {
    m_stream.close();
  }
}

SPtr<DataStream>
FileDataStream::clone() const {
  // Reopens the file fresh (cursor at start). Intended for read streams;
  // cloning a write stream re-truncates the file.
  return MakeShared<FileDataStream>(m_path, m_mode);
}

} // namespace sfmx
