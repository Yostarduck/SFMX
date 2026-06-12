#include "core/DataStream.h"

namespace sfmx
{

void
DataStream::writeString(StringView str) {
  SFMX_ASSERT(isWriteable());
  const uint64 length = static_cast<uint64>(str.size());
  write(&length, sizeof(length));
  if (length > 0) {
    write(str.data(), str.size());
  }
}

String
DataStream::readString() {
  SFMX_ASSERT(isReadable());
  uint64 length = 0;
  read(&length, sizeof(length));

  String out;
  if (length > 0) {
    out.resize(static_cast<size_t>(length));
    read(out.data(), static_cast<size_t>(length));
  }
  return out;
}

} // namespace sfmx
