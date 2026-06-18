#pragma once

#include "core/platform/Prerequisites.h"
#include "input/InputTypes.h"
#include "input/InputValue.h"
#include "input/Processor.h"

namespace sfmx
{

class InputSystem;

/** @brief Which output channel a composite part contributes to. */
enum class CompositeRole : int32 { kPositiveX, kNegativeX, kPositiveY, kNegativeY, kAxisX, kAxisY };

/** @brief Shape of a composite binding's combined value. */
enum class CompositeType : int32 { kAxis1D, kVector2D };

/**
 * @brief Addresses one concrete physical control (the leaf that sampling reads).
 *
 * For gamepads, @ref m_isAxis selects whether @ref m_code is an @ref Axis or a
 * @ref GamepadButton value; @ref m_gamepadIndex picks the pad (-1 == pad 0).
 */
struct InputControl
{
  DeviceType m_device = DeviceType::kKeyboard;
  int m_code = 0;
  int m_gamepadIndex = -1;
  bool m_isAxis = false;

  /** @brief Sample the current value: 1/0 for buttons, [-1,1] for axes. */
  NODISCARD float
  read(const InputSystem& system) const;

  /** @brief Human-readable path, e.g. "Keyboard/Space", "Gamepad0/Axis/LeftX". */
  NODISCARD String
  toPath() const;

  /** @brief Parse a control from the @ref toPath form. */
  NODISCARD static InputControl
  fromPath(StringView path);
};

/** @brief A single control feeding an action, with its own processor chain. */
struct Binding
{
  InputControl m_control;
  Vector<Processor> m_processors;
};

/** @brief One control contributing to a composite, routed by @ref m_role. */
struct BindingPart
{
  InputControl m_control;
  CompositeRole m_role = CompositeRole::kPositiveX;
  Vector<Processor> m_processors;
};

/** @brief Builds a higher-dimensional value from several controls. */
struct CompositeBinding
{
  CompositeType m_type = CompositeType::kVector2D;
  Vector<BindingPart> m_parts;

  /** @brief Combine the parts into a single value (x = pos - neg, etc.). */
  NODISCARD InputValue
  evaluate(const InputSystem& system) const;
};

} // namespace sfmx
