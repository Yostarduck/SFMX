#include "core/FileDataStream.h"

namespace sfmx
{

FileDataStream::FileDataStream(const FileSystemPath& path, AccessModeFlags mode)
  : DataStream(mode),
    m_path(path),
    m_readWrite(isReadable() && isWriteable()) {
  std::ios::openmode openMode = std::ios::binary;
  if (m_readWrite) {
    // in|out preserves existing content (file must exist); +trunc starts empty.
    openMode |= std::ios::in | std::ios::out;
    if (m_mode.isSetAny(AccessMode::kTruncate)) {
      openMode |= std::ios::trunc;
    }
  }
  else if (isWriteable()) {
    openMode |= std::ios::out | std::ios::trunc;
  }
  else {
    openMode |= std::ios::in;
  }

  m_stream.open(m_path, openMode);
  if (!m_stream.is_open()) {
    return;
  }

  if (isReadable()) {
    // Measure the file (0 for a freshly truncated one), then rewind. The seekg
    // also leaves the stream positioned and counts as the reposition before any
    // first write on a read+write stream.
    m_stream.seekg(0, std::ios::end);
    m_size = static_cast<size_t>(m_stream.tellg());
    m_stream.seekg(0, std::ios::beg);
  }
  else {
    m_size = 0;  // write-only: just truncated
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
  // On a read+write stream, switching from writing to reading needs an
  // intervening reposition (and clears any flag from a prior op).
  if (m_readWrite && m_lastOp == LastOp::kWrite) {
    m_stream.clear();
    m_stream.seekg(static_cast<std::streamoff>(m_pos), std::ios::beg);
  }
  m_stream.read(static_cast<char*>(dst), static_cast<std::streamsize>(bytes));
  const size_t got = static_cast<size_t>(m_stream.gcount());
  m_pos += got;  // advance by what was actually read (lockstep with the stream)
  m_lastOp = LastOp::kRead;
  return got;
}

size_t
FileDataStream::write(const void* src, size_t bytes) {
  if (!isWriteable() || !m_stream.is_open()) {
    return 0;
  }
  // On a read+write stream, switching from reading to writing needs an
  // intervening reposition (and clears the eof flag a read may have set).
  if (m_readWrite && m_lastOp == LastOp::kRead) {
    m_stream.clear();
    m_stream.seekp(static_cast<std::streamoff>(m_pos), std::ios::beg);
  }
  m_stream.write(static_cast<const char*>(src), static_cast<std::streamsize>(bytes));
  if (!m_stream.good()) {
    return 0;
  }
  m_pos += bytes;
  // Size grows only when the cursor passes the current end; overwriting the
  // middle (seek + write within bounds) leaves it unchanged.
  if (m_pos > m_size) {
    m_size = m_pos;
  }
  m_lastOp = LastOp::kWrite;
  return bytes;
}

void
FileDataStream::seek(size_t pos) {
  m_stream.clear();  // drop any eof/fail flag from a prior read
  if (isWriteable()) {
    // On a filebuf the get/put position is shared, so seekp also positions reads.
    m_stream.seekp(static_cast<std::streamoff>(pos), std::ios::beg);
  }
  else {
    m_stream.seekg(static_cast<std::streamoff>(pos), std::ios::beg);
  }
  m_pos = pos;
  // An explicit seek IS the reposition fstream needs, so either direction may
  // follow without an extra switch-seek.
  m_lastOp = LastOp::kNone;
}

void
FileDataStream::skip(int64 count) {
  // Resolve against our own cursor and reuse seek(), so m_pos and the stream
  // never drift apart on a relative move.
  int64 target = static_cast<int64>(m_pos) + count;
  if (target < 0) {
    target = 0;
  }
  seek(static_cast<size_t>(target));
}

size_t
FileDataStream::tell() const {
  return m_pos;
}

bool
FileDataStream::isAtEnd() const {
  return m_pos >= m_size;
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
