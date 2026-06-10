#pragma once

#include "core/platform/Prerequisites.h"
#include "input/InputTypes.h"
#include "input/InputValue.h"

namespace sfmx
{

/** @brief Kind of value transform a @ref Processor performs. */
namespace ProcessorType
{
enum E { kInvert, kScale, kDeadZone, kNormalize, kClamp };
} // namespace ProcessorType

/**
 * @brief A single value transform in a binding/action processor chain.
 *
 * A tagged POD evaluated without virtual dispatch. The meaning of the two
 * parameter vectors depends on @ref m_type:
 *  - kInvert:    m_paramA.x/.y are 0/1 flags to negate that component.
 *  - kScale:     m_paramA is the per-component multiplier.
 *  - kDeadZone:  m_paramA.x = min, m_paramA.y = max (applied to magnitude).
 *  - kNormalize: clamps a 2D vector's magnitude to <= 1 (no-op for scalars).
 *  - kClamp:     m_paramA = per-component min, m_paramB = per-component max.
 */
struct Processor
{
  ProcessorType::E m_type = ProcessorType::kScale;
  Vector2f m_paramA{1.f, 1.f};
  Vector2f m_paramB{0.f, 0.f};
};

/**
 * @brief Run @p value through @p processors in order and return the result.
 * @param type Interpretation of the value (scalar vs 2D), used by dead-zone /
 *             normalize to decide how to compute magnitude.
 */
NODISCARD Vector2f
applyProcessors(const Vector<Processor>& processors,
                Vector2f value,
                ActionValueType::E type);

} // namespace sfmx
