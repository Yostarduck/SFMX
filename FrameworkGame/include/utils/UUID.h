
#pragma once

#include "core/platform/Prerequisites.h"

#define STDUUID_FORCE_IMPLEMENTATION
#define UUID_SYSTEM_GENERATOR
#include "stduuid/uuid.h"

namespace sfmx {

class SFMX_UTILITY_EXPORT UUID  {
 public:
  UUID() = default;

  //explicit UUID(const String& str);

  NODISCARD FORCEINLINE bool
  isNull() const { return m_uuid.is_nil(); }

  NODISCARD FORCEINLINE String
  toString() const { return uuids::to_string(m_uuid); }

  NODISCARD FORCEINLINE bool
  operator==(const UUID& other) const { return m_uuid == other.m_uuid; }

  NODISCARD FORCEINLINE bool
  operator!=(const UUID& other) const { return m_uuid != other.m_uuid; }

  NODISCARD FORCEINLINE bool
  operator<(const UUID& other) const { return m_uuid < other.m_uuid; }

  NODISCARD FORCEINLINE size_t
  getHash() const noexcept { return Hash<uuids::uuid>{}(m_uuid); }

  NODISCARD static UUID
  createRandom();

  NODISCARD static UUID
  createFromName(const String& name, const UUID& namespace_uuid = UUID::null());

  NODISCARD FORCEINLINE static const UUID&
  null() { static UUID nullUUID(uuids::uuid{}); return nullUUID; }

 protected:

  explicit UUID(const uuids::uuid& uuid)
    : m_uuid(uuid) {}

 private:
  friend Hash<UUID>;
  uuids::uuid m_uuid;

  static const String CH_UUID_STR;
  static uuids::uuid_name_generator UUID_NAME_GENERATOR;
};

} // namespace sfmx

namespace std {
template <>
struct hash<sfmx::UUID> {
  using argument_type = sfmx::UUID;
  using result_type = sfmx::size_t;

  NODISCARD result_type
  operator()(const argument_type& uuid) const noexcept {
    return uuid.getHash();
  }

};
} // namespace std
