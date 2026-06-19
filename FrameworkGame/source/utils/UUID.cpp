
#include "utils/UUID.h"

#include <cstring>

namespace sfmx
{
const String UUID::SFMX_UUID_STR = "f81d4fae-7dec-11d0-a765-00a0c91e6bf6";
uuids::uuid_name_generator UUID::UUID_NAME_GENERATOR(uuids::uuid::from_string(SFMX_UUID_STR).value());

/*
*/
Array<uint8, kUuidBytes>
UUID::toBytes() const {
  Array<uint8, kUuidBytes> out{};
  const auto bytes = m_uuid.as_bytes();  // std::span<const std::byte, 16>
  std::memcpy(out.data(), bytes.data(), out.size());
  return out;
}

/*
*/
UUID
UUID::fromBytes(const Array<uint8, kUuidBytes>& bytes) {
  // Array<uint8,16> is std::array<uint8_t,16>, which uuids::uuid accepts directly.
  return UUID(uuids::uuid(bytes));
}

/*
*/
UUID
UUID::createRandom() {
  static thread_local std::mt19937 mt_rand(std::random_device{}());
  uuids::uuid_random_generator gen(mt_rand);
  return UUID(gen());
}

/*
*/
UUID
UUID::createFromName(const String& name, const UUID& namespace_uuid) {
  if (namespace_uuid.isNull()) {
    return UUID(UUID_NAME_GENERATOR(name));
  }
  else {
    uuids::uuid_name_generator generator(namespace_uuid.m_uuid);
    return UUID(generator(name));
  }
}
} // namespace chEngineSDK
