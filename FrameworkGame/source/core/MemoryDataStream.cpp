#include "core/MemoryDataStream.h"

#include <cstring>

namespace sfmx
{

MemoryDataStream::MemoryDataStream()
  : DataStream(AccessMode::kRead | AccessMode::kWrite)
{}

MemoryDataStream::MemoryDataStream(const void* data, size_t size)
  : DataStream(AccessMode::kRead | AccessMode::kWrite) {
  const uint8* bytes = static_cast<const uint8*>(data);
  m_data.assign(bytes, bytes + size);
  m_size = m_data.size();
}

MemoryDataStream::MemoryDataStream(const SPtr<DataStream>& source)
  : DataStream(AccessMode::kRead | AccessMode::kWrite) {
  SFMX_ASSERT(nullptr != source && source->isReadable());
  m_data.resize(source->size());
  if (!m_data.empty()) {
    const size_t read = source->read(m_data.data(), m_data.size());
    m_data.resize(read);
  }
  m_size = m_data.size();
}

MemoryDataStream::~MemoryDataStream() {
  close();
}

size_t
MemoryDataStream::read(void* dst, size_t bytes) {
  SFMX_ASSERT(isReadable());
  const size_t remaining = (m_pos < m_data.size()) ? (m_data.size() - m_pos) : 0;
  const size_t count = (bytes < remaining) ? bytes : remaining;
  if (count > 0) {
    std::memcpy(dst, m_data.data() + m_pos, count);
    m_pos += count;
  }
  return count;
}

size_t
MemoryDataStream::write(const void* src, size_t bytes) {
  SFMX_ASSERT(isWriteable());
  if (0 == bytes) {
    return 0;
  }
  if (m_pos + bytes > m_data.size()) {
    m_data.resize(m_pos + bytes);
  }
  std::memcpy(m_data.data() + m_pos, src, bytes);
  m_pos += bytes;
  m_size = m_data.size();
  return bytes;
}

void
MemoryDataStream::seek(size_t pos) {
  SFMX_ASSERT(pos <= m_data.size());
  m_pos = pos;
}

void
MemoryDataStream::skip(int64 count) {
  const int64 target = static_cast<int64>(m_pos) + count;
  SFMX_ASSERT(target >= 0 && static_cast<size_t>(target) <= m_data.size());
  m_pos = static_cast<size_t>(target);
}

size_t
MemoryDataStream::tell() const {
  return m_pos;
}

bool
MemoryDataStream::isAtEnd() const {
  return m_pos >= m_data.size();
}

void
MemoryDataStream::close() {
  m_data.clear();
  m_pos = 0;
  m_size = 0;
}

SPtr<DataStream>
MemoryDataStream::clone() const {
  return MakeShared<MemoryDataStream>(m_data.data(), m_data.size());
}

} // namespace sfmx
