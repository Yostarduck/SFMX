#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <SFML/Graphics/RenderWindow.hpp>

#include "core/platform/Prerequisites.h"
#include "utils/Module.h"

namespace sfmx
{

struct WindowCreateInfo
{
  std::string title;

  int32_t x {0};
  int32_t y {0};

  uint32_t width {0};
  uint32_t height {0};

  bool visible {true};
  bool resizable {true};
  bool decorated {true};
};

/**
 * @brief Cross-platform window backed by an @c sf::RenderWindow.
 *
 * SFML is itself cross-platform, so everything it can express (creation,
 * events, visibility, position/size, title, focus) is implemented here once.
 * Only the operations SFML has no API for -- native handle access, maximize /
 * minimize / restore, and runtime resize/decoration toggles -- are left as
 * hooks for a per-OS subclass (e.g. @ref Win32Window) to implement.
 *
 * A @ref Module (singleton): start it with @c Window::startUp(info), which
 * creates the concrete platform window immediately.
 */
class Window : public Module<Window>
{
 public:
  virtual ~Window() = default;

#pragma region Lifecycle
  /**
   * @brief Start the Window module, constructing the concrete window for the
   *        current platform and creating the native window immediately.
   *
   * Shadows Module::startUp so callers use the familiar @c Window::startUp(info)
   * form; the platform implementation is selected internally.
   */
  static void
  startUp(const WindowCreateInfo& createInfo);

  void
  destroy();

  bool
  isCreated() const;

  /** @brief Native OS window handle (implemented per platform). */
  virtual void*
  getNativeHandle() const = 0;

  /**
   * @brief Access the underlying SFML render window for drawing, events, and
   *        view control. Valid once the module has been started.
   */
  sf::RenderWindow&
  getRenderWindow();

  const sf::RenderWindow&
  getRenderWindow() const;
#pragma endregion

#pragma region Event processing
  void
  pollEvents();

  void
  requestClose();

  bool
  shouldClose() const;
#pragma endregion

#pragma region Visibility and focus
  void
  show();

  void
  hide();

  bool
  isVisible() const;

  void
  focus();

  bool
  hasFocus() const;
#pragma endregion

#pragma region Position and size
  void
  setPosition(const int32_t x, const int32_t y);

  void
  getPosition(int32_t& x, int32_t& y) const;

  void
  setSize(const uint32_t width, const uint32_t height);

  void
  getSize(uint32_t& width, uint32_t& height) const;

  void
  getFramebufferSize(uint32_t& width, uint32_t& height) const;
#pragma endregion

#pragma region Window state
  // No SFML API exists for these, so each platform implements them natively.
  virtual void
  maximize() = 0;

  virtual void
  minimize() = 0;

  virtual void
  restore() = 0;

  virtual bool
  isMaximized() const = 0;

  virtual bool
  isMinimized() const = 0;
#pragma endregion

#pragma region Window attributes
  void
  setTitle(const std::string_view title);

  std::string_view
  getTitle() const;

  void
  setResizable(const bool resizable);

  bool
  isResizable() const;

  void
  setDecorated(const bool decorated);

  bool
  isDecorated() const;

  /**
   * @brief Set the window icon from raw RGBA8 pixels (row-major, 4 bytes each).
   * @return @c false if the window is not created or @p rgba is null / the
   *         dimensions are zero; @c true once the icon has been applied.
   */
  bool
  setIcon(const uint32_t width, const uint32_t height, const uint8_t* rgba);
#pragma endregion

 protected:
  friend class Module<Window>;

  explicit Window(const WindowCreateInfo& createInfo)
    : m_createInfo(createInfo) {}

  // Module hook: create the native window once the module is started.
  void
  onStartUp() override;

  // Module hook: tear the native window down before the instance is freed.
  void
  onShutDown() override;

  // Apply the current resizable/decorated flags to the live window. SFML can
  // only set style at creation time, so platforms override this to update a
  // window that is already open. Default: no-op.
  virtual void
  applyStyle() {}

 private:
  bool
  createInternal(const WindowCreateInfo& createInfo);

  sf::RenderWindow m_window;

  std::string m_title;

  WindowCreateInfo m_createInfo;

  bool m_isVisible   {false};
  bool m_isResizable {true};
  bool m_isDecorated {true};
  bool m_shouldClose {false};
};

}
