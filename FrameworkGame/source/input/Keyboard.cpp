#include "input/Keyboard.h"

#include <SFML/Window/Event.hpp>

namespace sfmx
{

bool
Keyboard::isPressed(Key::E key) const {
  if (key < 0 || key >= Key::kCount) {
    return false;
  }
  return m_current.test(static_cast<size_t>(key));
}

bool
Keyboard::wasPressedThisFrame(Key::E key) const {
  if (key < 0 || key >= Key::kCount) {
    return false;
  }
  const size_t index = static_cast<size_t>(key);
  return m_current.test(index) && !m_previous.test(index);
}

bool
Keyboard::wasReleasedThisFrame(Key::E key) const {
  if (key < 0 || key >= Key::kCount) {
    return false;
  }
  const size_t index = static_cast<size_t>(key);
  return !m_current.test(index) && m_previous.test(index);
}

void
Keyboard::onEvent(const sf::Event& event) {
  if (const auto* pressed = event.getIf<sf::Event::KeyPressed>()) {
    const Key::E key = keyFromSfml(pressed->code);
    if (key >= 0 && key < Key::kCount) {
      m_current.set(static_cast<size_t>(key), true);
    }
  }
  else if (const auto* released = event.getIf<sf::Event::KeyReleased>()) {
    const Key::E key = keyFromSfml(released->code);
    if (key >= 0 && key < Key::kCount) {
      m_current.set(static_cast<size_t>(key), false);
    }
  }
}

void
Keyboard::beginFrame() {
  m_previous = m_current;
}

} // namespace sfmx
