#pragma once

#include "core/platform/Prerequisites.h"

namespace sfmx
{

/**
 * @brief Type-safe bit-flags wrapper around a scoped enumeration.
 *
 * `enum class` gives no bitwise operators, so combining flags is verbose and
 * error-prone. `Flags` stores the OR-ed bits in an integral @p Storage and
 * exposes the bitwise/query operations, while keeping the enum's type safety:
 * a `Flags<A>` never silently mixes with a `Flags<B>` or with a raw integer.
 *
 * Declare the free `(Enum, Enum)` operators next to the enum with
 * @ref SFMX_FLAGS_OPERATORS (or @ref SFMX_FLAGS_OPERATORS_EXT to pick the
 * storage type). The macro must be expanded in the **same namespace where the
 * enum is declared** so ADL finds the operators.
 *
 * @code
 * enum class AccessMode : uint16 {
 *   kNone  = 0x00,
 *   kRead  = 0x01,
 *   kWrite = 0x02,
 * };
 * SFMX_FLAGS_OPERATORS_EXT(AccessMode, uint16);
 * using AccessModeFlags = Flags<AccessMode, uint16>;
 *
 * AccessModeFlags mode = AccessMode::kRead | AccessMode::kWrite;
 * if (mode.isSetAny(AccessMode::kWrite)) { ... }
 * @endcode
 *
 * @tparam Enum    The scoped enumeration whose enumerators are single bits.
 * @tparam Storage Integral type holding the bits (defaults to uint32).
 */
template<typename Enum, typename Storage = uint32>
class Flags
{
 public:
  using EnumType = Enum;
  using InternalType = Storage;

  constexpr Flags() = default;
  constexpr Flags(const Flags&) = default;
  constexpr Flags(Enum value) : m_bits(static_cast<Storage>(value)) {}

  /** @brief Wrap a raw bit pattern. Explicit to avoid accidental int->Flags. */
  explicit constexpr Flags(Storage bits) : m_bits(bits) {}

  constexpr Flags& operator=(const Flags&) = default;

  FORCEINLINE constexpr Flags&
  operator=(Enum rhs) {
    m_bits = static_cast<Storage>(rhs);
    return *this;
  }

  // -- Queries ---------------------------------------------------------------
  /** @brief True if every bit in @p value is set. */
  NODISCARD FORCEINLINE constexpr bool
  isSet(Enum value) const {
    return (m_bits & static_cast<Storage>(value)) == static_cast<Storage>(value);
  }

  /** @brief True if every bit in @p value is set. */
  NODISCARD FORCEINLINE constexpr bool
  isSet(const Flags& value) const {
    return (m_bits & value.m_bits) == value.m_bits;
  }

  /** @brief True if any bit in @p value is set. */
  NODISCARD FORCEINLINE constexpr bool
  isSetAny(Enum value) const {
    return (m_bits & static_cast<Storage>(value)) != 0;
  }

  /** @brief True if any bit in @p value is set. */
  NODISCARD FORCEINLINE constexpr bool
  isSetAny(const Flags& value) const {
    return (m_bits & value.m_bits) != 0;
  }

  // -- Mutators --------------------------------------------------------------
  /** @brief Activate every bit in @p value. */
  FORCEINLINE constexpr Flags&
  set(Enum value) {
    m_bits |= static_cast<Storage>(value);
    return *this;
  }

  /** @brief Clear every bit in @p value. */
  FORCEINLINE constexpr Flags&
  unset(Enum value) {
    m_bits &= static_cast<Storage>(~static_cast<Storage>(value));
    return *this;
  }

  // -- Comparison ------------------------------------------------------------
  // C++20 synthesizes operator!= and the reversed (enum == flags) forms.
  NODISCARD FORCEINLINE constexpr bool
  operator==(const Flags& rhs) const {
    return m_bits == rhs.m_bits;
  }

  NODISCARD FORCEINLINE constexpr bool
  operator==(Enum rhs) const {
    return m_bits == static_cast<Storage>(rhs);
  }

  // -- Bitwise: OR -----------------------------------------------------------
  FORCEINLINE constexpr Flags&
  operator|=(Enum rhs) {
    m_bits |= static_cast<Storage>(rhs);
    return *this;
  }

  FORCEINLINE constexpr Flags&
  operator|=(const Flags& rhs) {
    m_bits |= rhs.m_bits;
    return *this;
  }

  NODISCARD FORCEINLINE constexpr Flags
  operator|(Enum rhs) const {
    return Flags(*this) |= rhs;
  }

  NODISCARD FORCEINLINE constexpr Flags
  operator|(const Flags& rhs) const {
    return Flags(*this) |= rhs;
  }

  // -- Bitwise: AND ----------------------------------------------------------
  FORCEINLINE constexpr Flags&
  operator&=(Enum rhs) {
    m_bits &= static_cast<Storage>(rhs);
    return *this;
  }

  FORCEINLINE constexpr Flags&
  operator&=(const Flags& rhs) {
    m_bits &= rhs.m_bits;
    return *this;
  }

  NODISCARD FORCEINLINE constexpr Flags
  operator&(Enum rhs) const {
    return Flags(*this) &= rhs;
  }

  NODISCARD FORCEINLINE constexpr Flags
  operator&(const Flags& rhs) const {
    return Flags(*this) &= rhs;
  }

  // -- Bitwise: XOR ----------------------------------------------------------
  FORCEINLINE constexpr Flags&
  operator^=(Enum rhs) {
    m_bits ^= static_cast<Storage>(rhs);
    return *this;
  }

  FORCEINLINE constexpr Flags&
  operator^=(const Flags& rhs) {
    m_bits ^= rhs.m_bits;
    return *this;
  }

  NODISCARD FORCEINLINE constexpr Flags
  operator^(Enum rhs) const {
    return Flags(*this) ^= rhs;
  }

  NODISCARD FORCEINLINE constexpr Flags
  operator^(const Flags& rhs) const {
    return Flags(*this) ^= rhs;
  }

  // -- Bitwise: NOT ----------------------------------------------------------
  NODISCARD FORCEINLINE constexpr Flags
  operator~() const {
    return Flags(static_cast<Storage>(~m_bits));
  }

  /** @brief enum-on-the-left AND, e.g. `AccessMode::kRead & someFlags`. */
  NODISCARD friend FORCEINLINE constexpr Flags
  operator&(Enum lhs, const Flags& rhs) {
    return Flags(static_cast<Storage>(lhs) & rhs.m_bits);
  }

  // -- Conversion ------------------------------------------------------------
  /** @brief Explicit, so `if (flags)` works but accidental int math does not. */
  NODISCARD explicit FORCEINLINE constexpr
  operator bool() const {
    return 0 != m_bits;
  }

  /** @brief The raw bit pattern (for serialization / interop). */
  NODISCARD FORCEINLINE constexpr Storage
  getRaw() const {
    return m_bits;
  }

 private:
  Storage m_bits{0};
};

/**
 * @brief Define the free `(Enum, Enum)` bitwise operators for a flag enum,
 *        with an explicit @p Storage type.
 *
 * Expand in the SAME namespace where @p Enum is declared (ADL).
 */
#define SFMX_FLAGS_OPERATORS_EXT(Enum, Storage)                                \
  FORCEINLINE constexpr ::sfmx::Flags<Enum, Storage>                           \
  operator|(Enum a, Enum b) {                                                  \
    return ::sfmx::Flags<Enum, Storage>(a) | b;                                \
  }                                                                            \
  FORCEINLINE constexpr ::sfmx::Flags<Enum, Storage>                           \
  operator&(Enum a, Enum b) {                                                  \
    return ::sfmx::Flags<Enum, Storage>(a) & b;                                \
  }                                                                            \
  FORCEINLINE constexpr ::sfmx::Flags<Enum, Storage>                           \
  operator^(Enum a, Enum b) {                                                  \
    return ::sfmx::Flags<Enum, Storage>(a) ^ b;                                \
  }                                                                            \
  FORCEINLINE constexpr ::sfmx::Flags<Enum, Storage>                           \
  operator~(Enum a) {                                                          \
    return ~::sfmx::Flags<Enum, Storage>(a);                                   \
  }

/** @brief Like @ref SFMX_FLAGS_OPERATORS_EXT with uint32 storage. */
#define SFMX_FLAGS_OPERATORS(Enum) SFMX_FLAGS_OPERATORS_EXT(Enum, ::sfmx::uint32)

/**
 * @brief Declare a `Flags` alias @p AliasName for @p Enum AND its `(Enum, Enum)`
 *        operators, in one line, with an explicit @p Storage type.
 *
 * The alias name is explicit (not derived) because the convention varies
 * (`AccessMode` -> `AccessModeFlags`, `TestFlag` -> `TestFlags`). Expand in the
 * SAME namespace where @p Enum is declared (ADL, see @ref SFMX_FLAGS_OPERATORS_EXT).
 *
 * @code
 * enum class AccessMode : uint16 { kNone = 0, kRead = 1, kWrite = 2 };
 * SFMX_DECLARE_FLAGS_EXT(AccessModeFlags, AccessMode, uint16)
 * // -> using AccessModeFlags = Flags<AccessMode, uint16>;  + the operators
 * @endcode
 */
#define SFMX_DECLARE_FLAGS_EXT(AliasName, Enum, Storage)                       \
  using AliasName = ::sfmx::Flags<Enum, Storage>;                              \
  SFMX_FLAGS_OPERATORS_EXT(Enum, Storage)

/** @brief Like @ref SFMX_DECLARE_FLAGS_EXT with uint32 storage. */
#define SFMX_DECLARE_FLAGS(AliasName, Enum)                                    \
  SFMX_DECLARE_FLAGS_EXT(AliasName, Enum, ::sfmx::uint32)

} // namespace sfmx
