#include "core/Window.h"

#if SFMX_PLATFORM_WINDOWS
  #include "core/Window_Win32.h"
#elif SFMX_PLATFORM_LINUX
  #include "core/Window_Linux.h"
#elif SFMX_PLATFORM_MACOS
  // TODO: include the macOS window implementation once it exists.
#endif

#include <SFML/Window/Event.hpp>

namespace sfmx
{

namespace
{

// SFML window style flags for a given configuration, applied at creation time.
std::uint32_t
sfmlStyle(const bool resizable, const bool decorated)
{
  if (!decorated) {
    return sf::Style::None;
  }

  std::uint32_t style = sf::Style::Titlebar | sf::Style::Close;
  if (resizable) {
    style |= sf::Style::Resize;
  }
  return style;
}

} // namespace

void
Window::startUp(const WindowCreateInfo& createInfo)
{
#if SFMX_PLATFORM_WINDOWS
  Module<Window>::startUp<Win32Window>(createInfo);
#elif SFMX_PLATFORM_LINUX
  Module<Window>::startUp<LinuxWindow>(createInfo);
#elif SFMX_PLATFORM_MACOS
  #error "Window: no macOS implementation available yet."
#else
  #error "Window: unsupported platform."
#endif
}

void
Window::onStartUp()
{
  createInternal(m_createInfo);
}

void
Window::onShutDown()
{
  destroy();
}

bool
Window::createInternal(const WindowCreateInfo& createInfo)
{
  m_title       = createInfo.title;
  m_isVisible   = createInfo.visible;
  m_isResizable = createInfo.resizable;
  m_isDecorated = createInfo.decorated;
  m_shouldClose = false;

  const sf::VideoMode   mode(sf::Vector2u(createInfo.width, createInfo.height));
  const std::uint32_t   style = sfmlStyle(m_isResizable, m_isDecorated);

  m_window.create(mode,
                  sf::String::fromUtf8(m_title.begin(), m_title.end()),
                  style);

  if (!m_window.isOpen()) {
    return false;
  }

  m_window.setPosition(sf::Vector2i(createInfo.x, createInfo.y));
  m_window.setVisible(m_isVisible);

  return true;
}

void
Window::destroy()
{
  if (m_window.isOpen()) {
    m_window.close();
  }

  m_title.clear();
  m_isVisible   = false;
  m_isResizable = true;
  m_isDecorated = true;
  m_shouldClose = false;
}

NODISCARD bool
Window::isCreated() const
{
  return m_window.isOpen();
}

sf::RenderWindow&
Window::getRenderWindow()
{
  return m_window;
}

const sf::RenderWindow&
Window::getRenderWindow() const
{
  return m_window;
}

void
Window::pollEvents()
{
  while (const std::optional<sf::Event> event = m_window.pollEvent()) {
    if (event->is<sf::Event::Closed>()) {
      m_shouldClose = true;
    }
  }
}

void
Window::requestClose()
{
  m_shouldClose = true;
}

bool
Window::shouldClose() const
{
  return m_shouldClose;
}

void
Window::show()
{
  if (isCreated() && !m_isVisible) {
    m_window.setVisible(true);
    m_isVisible = true;
  }
}

void
Window::hide()
{
  if (isCreated() && m_isVisible) {
    m_window.setVisible(false);
    m_isVisible = false;
  }
}

NODISCARD bool
Window::isVisible() const
{
  return m_isVisible;
}

void
Window::focus()
{
  if (isCreated()) {
    m_window.requestFocus();
  }
}

NODISCARD bool
Window::hasFocus() const
{
  return isCreated() && m_window.hasFocus();
}

// -- Position and size --------------------------------------------------
void
Window::setPosition(const int32_t x, const int32_t y)
{
  if (isCreated()) {
    m_window.setPosition(sf::Vector2i(x, y));
  }
}

void
Window::getPosition(int32_t& x, int32_t& y) const
{
  const sf::Vector2i position = m_window.getPosition();
  x = position.x;
  y = position.y;
}

void
Window::setSize(const uint32_t width, const uint32_t height)
{
  if (isCreated()) {
    m_window.setSize(sf::Vector2u(width, height));
  }
}

void
Window::getSize(uint32_t& width, uint32_t& height) const
{
  const sf::Vector2u size = m_window.getSize();
  width  = size.x;
  height = size.y;
}

void
Window::getFramebufferSize(uint32_t& width, uint32_t& height) const
{
  // SFML reports the drawable (client) area directly.
  const sf::Vector2u size = m_window.getSize();
  width  = size.x;
  height = size.y;
}

void
Window::setTitle(const std::string_view title)
{
  m_title = title;
  if (isCreated()) {
    m_window.setTitle(sf::String::fromUtf8(m_title.begin(), m_title.end()));
  }
}

NODISCARD std::string_view
Window::getTitle() const
{
  return m_title;
}

void
Window::setResizable(const bool resizable)
{
  m_isResizable = resizable;
  applyStyle();
}

NODISCARD bool
Window::isResizable() const
{
  return m_isResizable;
}

void
Window::setDecorated(const bool decorated)
{
  m_isDecorated = decorated;
  applyStyle();
}

NODISCARD bool
Window::isDecorated() const
{
  return m_isDecorated;
}

NODISCARD bool
Window::setIcon(const uint32_t width, const uint32_t height, const uint8_t* rgba)
{
  if (!isCreated() || nullptr == rgba || 0 == width || 0 == height) {
    return false;
  }

  m_window.setIcon(sf::Vector2u(width, height), rgba);
  return true;
}

}
