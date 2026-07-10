#pragma once

#include "core/Window.h"

namespace sfmx
{

/**
 * @brief Linux @ref Window: adds the native operations SFML has no API for.
 *
 * All the SFML-backed behavior lives in @ref Window. This subclass only wires
 * up the pieces that require the native X11 window -- handle access, maximize /
 * minimize / restore, and runtime resize/decoration style changes, driven
 * through the window manager (EWMH / Motif hints).
 */
class LinuxWindow : public Window
{
 public:

  ~LinuxWindow() override = default;

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

  explicit LinuxWindow(const WindowCreateInfo& createInfo)
    : Window(createInfo) {}

  // Native X11 window handle of the render window (0 while the window is closed).
  sf::WindowHandle
  nativeHandle() const;
};

} // namespace sfmx
