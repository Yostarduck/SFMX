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
#include "core/utils/UUID.h"

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

#define DECLARE_TYPE_TRAITS(TypeClass)                                                        \
  template <> struct TypeTraits<TypeClass> {                                                  \
    static constexpr const ansichar*                                                          \
    getTypeName() {                                                                           \
      return #TypeClass;                                                                      \
    }                                                                                         \
    static const UUID&                                                                        \
    getTypeId() {                                                                             \
      static const UUID typeId = UUID::createFromName(#TypeClass);                            \
      return typeId;                                                                          \
    }                                                                                         \
  };

#define DECLARE_TYPE_TRAITS_NAMESPACE_ID(NameSpaceIdExpr, TypeClass)                          \
  template <> struct TypeTraits<TypeClass> {                                                  \
    static constexpr const ansichar*                                                          \
    getTypeName() {                                                                           \
      return #TypeClass;                                                                      \
    }                                                                                         \
    static const UUID&                                                                        \
    getTypeId() {                                                                             \
      static const UUID typeId = UUID::createFromName(#TypeClass, NameSpaceIdExpr);           \
      return typeId;                                                                          \
    }                                                                                         \
  };
} // namespace sfmx
