#pragma once

#include <cmath>

#include "core/platform/Prerequisites.h"
#include "input/InputTypes.h"

namespace sfmx
{

/** @brief The dimensionality / interpretation of an action's value. */
namespace ActionValueType { enum E { kButton, kAxis1D, kAxis2D }; }

/**
 * @brief A small POD value carried by actions and contexts; holds bool, float,
 *        or Vector2 without the churn of a runtime variant.
 *
 * @c x backs bool (non-zero == true) and 1D values; @c x and @c y back 2D.
 */
struct InputValue
{
  ActionValueType::E m_type = ActionValueType::kButton;
  Vector2f m_value{0.f, 0.f};

  NODISCARD bool
  asBool() const { return m_value.x != 0.f; }

  NODISCARD float
  asFloat() const { return m_value.x; }

  NODISCARD Vector2f
  asVector2() const { return m_value; }

  /** @brief Scalar magnitude used to decide whether the action is actuated. */
  NODISCARD float
  magnitude() const {
    if (ActionValueType::kAxis2D == m_type) {
      return std::sqrt(m_value.x * m_value.x + m_value.y * m_value.y);
    }
    return std::fabs(m_value.x);
  }
};

} // namespace sfmx
