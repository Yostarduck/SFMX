#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "core/Window.h"

namespace sfmx
{

/**
 * @brief Windows @ref Window: adds the native operations SFML has no API for.
 *
 * All the SFML-backed behavior lives in @ref Window. This subclass only wires
 * up the pieces that require the native @c HWND -- handle access, maximize /
 * minimize / restore, and runtime resize/decoration style changes.
 */
class Win32Window : public Window
{
 public:

  ~Win32Window() override = default;

  NODISCARD void*
  getNativeHandle() const override;
  
  void
  maximize() override;

  void
  minimize() override;

  void
  restore() override;

  NODISCARD bool
  isMaximized() const override;

  NODISCARD bool
  isMinimized() const override;

 protected:
  // Push the current resize/decoration flags onto the native window style.
  void
  applyStyle() override;

 private:
  friend class Module<Window>;

  explicit Win32Window(const WindowCreateInfo& createInfo)
    : Window(createInfo) {}

  // Native handle of the render window (nullptr while the window is closed).
  HWND
  nativeHandle() const;
};

} // namespace sfmx
