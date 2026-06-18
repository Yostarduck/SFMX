#pragma once

// Serialization overloads for framework / non-trivially-copyable types, kept in
// one place so the format for each type lives here and new types extend it
// without touching DataStream itself.
//
// INCLUDE THIS HEADER (not core/DataStream.h directly) when you serialize
// UUID / Vector / future framework types. DataStream.h alone only provides the
// raw POD operator<< / operator>> and writeString/readString.
//
// Note: UUID is trivially-copyable, so `stream << uuid` without this header in
// scope would silently fall back to DataStream's raw-blit member operator
// instead of the 16-byte canonical form below. Always reach for this header to
// serialize, so the explicit overload is the one selected.

#include "core/DataStream.h"
#include "utils/UUID.h"

namespace sfmx
{

// -- UUID: raw 16 bytes (compact, endianness-free, layout-independent) --------
FORCEINLINE DataStream&
operator<<(DataStream& stream, const UUID& id) {
  SFMX_ASSERT(stream.isWriteable());
  const Array<uint8, kUuidBytes> bytes = id.toBytes();
  stream.write(bytes.data(), bytes.size());
  return stream;
}

FORCEINLINE DataStream&
operator>>(DataStream& stream, UUID& id) {
  SFMX_ASSERT(stream.isReadable());
  Array<uint8, kUuidBytes> bytes{};
  stream.read(bytes.data(), bytes.size());
  id = UUID::fromBytes(bytes);
  return stream;
}

// -- Vector<T> (trivially-copyable T): [uint64 count][count*sizeof(T)] --------
// These are more specialized than DataStream's member operator<< template, so
// `stream << vec` resolves here.
template<typename T>
DataStream&
operator<<(DataStream& stream, const Vector<T>& values) {
  static_assert(std::is_trivially_copyable_v<T>,
                "Vector<T> bulk serialization needs a trivially-copyable T; "
                "serialize a vector of non-trivial elements one by one.");
  const uint64 count = static_cast<uint64>(values.size());
  stream.write(&count, sizeof(count));
  if (!values.empty()) {
    stream.write(values.data(), values.size() * sizeof(T));
  }
  return stream;
}

template<typename T>
DataStream&
operator>>(DataStream& stream, Vector<T>& values) {
  static_assert(std::is_trivially_copyable_v<T>,
                "Vector<T> bulk deserialization needs a trivially-copyable T; "
                "deserialize a vector of non-trivial elements one by one.");
  uint64 count = 0;
  stream.read(&count, sizeof(count));
  values.resize(static_cast<size_t>(count));
  if (count > 0) {
    stream.read(values.data(), static_cast<size_t>(count) * sizeof(T));
  }
  return stream;
}

} // namespace sfmx
