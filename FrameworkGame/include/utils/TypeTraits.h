/************************************************************************/
/**
 * @file chTypeTraits.h
 * @author AccelMR
 * @date 2025/07/12
 * @brief  Asset type traits for Chimera Core.
 */
/************************************************************************/
#pragma once

#include "core/platform/PlatformTypes.h"
#include "utils/UUID.h"

namespace sfmx {

template <typename T> struct TypeTraits {
  static constexpr const ansichar*
  getTypeName() {
    return "Unknown";
  }
  static const UUID&
  getTypeId() {
    return UUID::null();
  }
};

} // namespace sfmx

#define DECLARE_TYPE_TRAITS(TypeClass)                                                        \
  template <> struct sfmx::TypeTraits<TypeClass> {                                            \
    static constexpr const sfmx::ansichar*                                                    \
    getTypeName() {                                                                           \
      return #TypeClass;                                                                      \
    }                                                                                         \
    static const sfmx::UUID&                                                                  \
    getTypeId() {                                                                             \
      static const sfmx::UUID typeId = sfmx::UUID::createFromName(#TypeClass);                \
      return typeId;                                                                          \
    }                                                                                         \
  };

#define DECLARE_TYPE_TRAITS_NAMESPACE_ID(NameSpaceIdExpr, TypeClass)                          \
  template <> struct sfmx::TypeTraits<TypeClass> {                                            \
    static constexpr const sfmx::ansichar*                                                    \
    getTypeName() {                                                                           \
      return #TypeClass;                                                                      \
    }                                                                                         \
    static const sfmx::UUID&                                                                  \
    getTypeId() {                                                                             \
      static const sfmx::UUID typeId = sfmx::UUID::createFromName(#TypeClass, NameSpaceIdExpr); \
      return typeId;                                                                          \
    }                                                                                         \
  };
