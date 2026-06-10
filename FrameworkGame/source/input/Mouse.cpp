#include "input/Mouse.h"

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/WindowBase.hpp>

namespace sfmx
{

bool
Mouse::isPressed(MouseButton::E button) const {
  if (button < 0 || button >= MouseButton::kCount) {
    return false;
  }
  return m_current.test(static_cast<size_t>(button));
}

bool
Mouse::wasPressedThisFrame(MouseButton::E button) const {
  if (button < 0 || button >= MouseButton::kCount) {
    return false;
  }
  const size_t index = static_cast<size_t>(button);
  return m_current.test(index) && !m_previous.test(index);
}

bool
Mouse::wasReleasedThisFrame(MouseButton::E button) const {
  if (button < 0 || button >= MouseButton::kCount) {
    return false;
  }
  const size_t index = static_cast<size_t>(button);
  return !m_current.test(index) && m_previous.test(index);
}

Vector2i
Mouse::getPosition() const {
  return m_position;
}

float
Mouse::getWheelDelta() const {
  return m_wheelDelta;
}

void
Mouse::onEvent(const sf::Event& event) {
  if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>()) {
    const MouseButton::E button = mouseButtonFromSfml(pressed->button);
    m_current.set(static_cast<size_t>(button), true);
  }
  else if (const auto* released = event.getIf<sf::Event::MouseButtonReleased>()) {
    const MouseButton::E button = mouseButtonFromSfml(released->button);
    m_current.set(static_cast<size_t>(button), false);
  }
  else if (const auto* scrolled = event.getIf<sf::Event::MouseWheelScrolled>()) {
    if (scrolled->wheel == sf::Mouse::Wheel::Vertical) {
      m_wheelDelta += scrolled->delta;
    }
  }
}

void
Mouse::pollPosition(const sf::WindowBase& window) {
  m_position = sf::Mouse::getPosition(window);
}

void
Mouse::beginFrame() {
  m_previous = m_current;
  m_wheelDelta = 0.f;
}

} // namespace sfmx
