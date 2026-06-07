#pragma once

#include <SFML/System/Vector2.hpp>

#include "core/platform/Prerequisites.h"

// Forward declarations of the SFML input enums so consumers of this header stay
// free of the heavy <SFML/Window/...> includes. SFML declares these as scoped
// enums with the default (int) underlying type, so a forward declaration is
// well-defined. The real includes live only in the input .cpp files.
namespace sf
{
namespace Keyboard { enum class Key; }
namespace Mouse    { enum class Button; }
namespace Joystick { enum class Axis; }
} // namespace sf

namespace sfmx
{

/** @brief 2D float vector used for input values (alias of SFML's, see Transform.h). */
using Vector2f = sf::Vector2f;
/** @brief 2D int vector used for mouse positions. */
using Vector2i = sf::Vector2i;

/** @brief Which physical device a control belongs to. */
namespace DeviceType { enum E { kKeyboard, kMouse, kGamepad, kCount }; }

/**
 * @brief Keyboard keys, mirroring the order of sf::Keyboard::Key so conversion
 *        at the boundary is a checked cast. @c kCount equals SFML's KeyCount.
 */
namespace Key
{
enum E
{
  kUnknown = -1,
  kA = 0, kB, kC, kD, kE, kF, kG, kH, kI, kJ, kK, kL, kM,
  kN, kO, kP, kQ, kR, kS, kT, kU, kV, kW, kX, kY, kZ,
  kNum0, kNum1, kNum2, kNum3, kNum4, kNum5, kNum6, kNum7, kNum8, kNum9,
  kEscape, kLControl, kLShift, kLAlt, kLSystem,
  kRControl, kRShift, kRAlt, kRSystem, kMenu,
  kLBracket, kRBracket, kSemicolon, kComma, kPeriod, kApostrophe,
  kSlash, kBackslash, kGrave, kEqual, kHyphen,
  kSpace, kEnter, kBackspace, kTab, kPageUp, kPageDown, kEnd, kHome,
  kInsert, kDelete, kAdd, kSubtract, kMultiply, kDivide,
  kLeft, kRight, kUp, kDown,
  kNumpad0, kNumpad1, kNumpad2, kNumpad3, kNumpad4,
  kNumpad5, kNumpad6, kNumpad7, kNumpad8, kNumpad9,
  kF1, kF2, kF3, kF4, kF5, kF6, kF7, kF8, kF9, kF10,
  kF11, kF12, kF13, kF14, kF15, kPause,
  kCount
};
} // namespace Key

/** @brief Mouse buttons, mirroring sf::Mouse::Button. */
namespace MouseButton { enum E { kLeft, kRight, kMiddle, kExtra1, kExtra2, kCount }; }

/**
 * @brief Semantic gamepad axes. Mapped to concrete sf::Joystick::Axis at the
 *        boundary; the trigger / right-stick mapping is pad-dependent and may
 *        need per-device tuning (see toSfml in InputTypes.cpp).
 */
namespace Axis
{
enum E { kLeftX, kLeftY, kRightX, kRightY, kLeftTrigger, kRightTrigger,
         kPovX, kPovY, kCount };
} // namespace Axis

/**
 * @brief Semantic gamepad buttons. Mapped to raw SFML button indices (0-based)
 *        in declaration order; concrete layout varies by controller.
 */
namespace GamepadButton
{
enum E { kSouth, kEast, kWest, kNorth, kL1, kR1,
         kSelect, kStart, kLStick, kRStick, kCount };
} // namespace GamepadButton

/** @brief Maximum number of simultaneously tracked gamepads (sf::Joystick::Count). */
constexpr int kMaxGamepads = 8;
/** @brief Default analog dead-zone, as a fraction of full deflection. */
constexpr float kDefaultDeadZone = 0.15f;
/** @brief Default seconds a Hold interaction must be held to perform. */
constexpr float kDefaultHoldTime = 0.4f;
/** @brief Default maximum seconds between taps in a MultiTap interaction. */
constexpr float kDefaultTapSpacing = 0.2f;

/************************************************************************/
/* SFML boundary conversions (defined in InputTypes.cpp)                */
/************************************************************************/

NODISCARD sf::Keyboard::Key toSfml(Key::E key);
NODISCARD Key::E            keyFromSfml(sf::Keyboard::Key key);

NODISCARD sf::Mouse::Button toSfml(MouseButton::E button);
NODISCARD MouseButton::E    mouseButtonFromSfml(sf::Mouse::Button button);

NODISCARD sf::Joystick::Axis toSfml(Axis::E axis);

NODISCARD int           toSfmlButton(GamepadButton::E button);
NODISCARD GamepadButton::E gamepadButtonFromSfml(int button);

/************************************************************************/
/* Stable string names for serialization (defined in InputTypes.cpp)    */
/************************************************************************/

NODISCARD String toString(Key::E key);
NODISCARD Key::E keyFromString(StringView name);

NODISCARD String toString(MouseButton::E button);
NODISCARD MouseButton::E mouseButtonFromString(StringView name);

NODISCARD String toString(Axis::E axis);
NODISCARD Axis::E axisFromString(StringView name);

NODISCARD String toString(GamepadButton::E button);
NODISCARD GamepadButton::E gamepadButtonFromString(StringView name);

NODISCARD String toString(DeviceType::E device);
NODISCARD DeviceType::E deviceTypeFromString(StringView name);

} // namespace sfmx
