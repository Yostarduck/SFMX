#include "ui/UIEventSystem.h"
#include "input/InputAction.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace sfmx
{

// -- Lifecycle ---------------------------------------------------------------

void UIEventSystem::onStartUp() {
  m_canvases.clear();
  m_selected = nullptr;
  m_pointer = PointerState{};
  m_navigateAction = nullptr;
  m_submitAction = nullptr;
  m_cancelAction = nullptr;
  m_navTimer = 0.f;
  m_navHeld = false;
}

void UIEventSystem::onShutDown() {
  m_canvases.clear();
  m_selected = nullptr;
  m_navigateAction = nullptr;
  m_submitAction = nullptr;
  m_cancelAction = nullptr;
}

void UIEventSystem::update(const sf::WindowBase& window, float deltaTime) {
  validateSelection();
  processPointer(window);
  processNavigation(deltaTime);
}

// -- Canvas registry ---------------------------------------------------------

void UIEventSystem::registerCanvas(Canvas* canvas) {
  if (canvas == nullptr) {
    return;
  }

  auto it = m_canvases.begin();
  while (it != m_canvases.end() && (*it)->getDepth() <= canvas->getDepth()) {
    ++it;
  }
  m_canvases.insert(it, canvas);
}

void UIEventSystem::unregisterCanvas(Canvas* canvas) {
  auto it = std::find(m_canvases.begin(), m_canvases.end(), canvas);
  if (it != m_canvases.end()) {
    m_canvases.erase(it);
  }
}

// -- Selection ---------------------------------------------------------------

void UIEventSystem::setSelected(UIWidget* widget) {
  if (m_selected == widget) {
    return;
  }

  if (m_selected != nullptr) {
    m_selected->setFocused(false);
    m_selected->onDeselect();
  }

  m_selected = widget;

  if (m_selected != nullptr) {
    m_selected->setFocused(true);
    m_selected->onSelect();
  }
}

// -- InputAction integration -------------------------------------------------

void UIEventSystem::setSubmitAction(InputAction* action) {
  m_submitAction = action;
  m_submitSub = {};  // disconnect previous

  if (action != nullptr) {
    m_submitSub = action->onPerformed([this](const InputContext& ctx) {
      SFMX_PARAMETER_UNUSED(ctx);
      if (m_selected != nullptr) {
        m_selected->onSubmit();
      }
    });
  }
}

void UIEventSystem::setCancelAction(InputAction* action) {
  m_cancelAction = action;
  m_cancelSub = {};  // disconnect previous

  if (action != nullptr) {
    m_cancelSub = action->onPerformed([this](const InputContext& ctx) {
      SFMX_PARAMETER_UNUSED(ctx);
      if (m_selected != nullptr) {
        m_selected->onCancel();
      }
    });
  }
}

// -- Internal ----------------------------------------------------------------

void UIEventSystem::validateSelection() {
  if (m_selected == nullptr) {
    return;
  }

  if (!m_selected->isEnabled() || !m_selected->isVisible() ||
      m_selected->getCanvas() == nullptr) {
    setSelected(nullptr);
  }
}

void UIEventSystem::processPointer(const sf::WindowBase& window) {
  const sf::Vector2i screenPos = sf::Mouse::getPosition(window);

  if (m_canvases.empty()) {
    m_pointer.screenPos = screenPos;
    m_pointer.hovered = nullptr;
    m_pointer.canvasPos = {};
    return;
  }

  Canvas* topCanvas = m_canvases.back();

  const sf::Transform inv = topCanvas->getTransform().getInverse();
  const sf::Vector2f canvasPos = inv.transformPoint(
    static_cast<sf::Vector2f>(screenPos));

  m_pointer.screenPos = screenPos;
  m_pointer.canvasPos = canvasPos;

  UIWidget* hit = topCanvas->hitTest(canvasPos);

  // -- Enter / Exit --------------------------------------------------------
  if (hit != m_pointer.hovered) {
    if (m_pointer.hovered != nullptr) {
      m_pointer.hovered->onPointerExit(canvasPos);
    }

    m_pointer.hovered = hit;

    if (hit != nullptr) {
      hit->onPointerEnter(canvasPos);
    }
  }

  // -- Button state --------------------------------------------------------
  const bool isDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);

  if (isDown && !m_pointer.buttonDown) {
    m_pointer.buttonDown = true;
    m_pointer.pressed = hit;

    if (hit != nullptr) {
      hit->onPointerDown(canvasPos);
      setSelected(hit);
    }
  } 
  else if (!isDown && m_pointer.buttonDown) {
    m_pointer.buttonDown = false;

    if (m_pointer.pressed != nullptr) {
      m_pointer.pressed->onPointerUp(canvasPos);

      if (m_pointer.pressed == hit && hit != nullptr) {
        hit->onPointerClick(canvasPos);
      }
    }

    m_pointer.pressed = nullptr;
  }
}

void UIEventSystem::processNavigation(float deltaTime) {
  if (m_navigateAction == nullptr) {
    return;
  }

  const InputValue& navValue = m_navigateAction->getValue();
  const sf::Vector2f dir = navValue.asVector2();

  // Deadzone
  constexpr float kDeadzone = 0.5f;
  const float mag = std::sqrt(dir.x * dir.x + dir.y * dir.y);
  if (mag < kDeadzone) {
    m_navTimer = 0.f;
    m_navHeld = false;
    return;
  }

  const sf::Vector2f normDir = dir / mag;

  if (!m_navHeld) {
    // First press — move immediately
    m_navHeld = true;
    m_navTimer = 0.f;
    moveSelection(normDir);
  } 
  else {
    // Held — use cooldown (initial delay, then repeat)
    constexpr float kInitialDelay = 0.4f;
    constexpr float kRepeatRate = 0.15f;

    m_navTimer += deltaTime;

    if (m_navTimer >= kInitialDelay) {
      // How long since the repeat window started
      const float sinceRepeat = m_navTimer - kInitialDelay;
      const int prevStep = static_cast<int>((sinceRepeat - deltaTime) / kRepeatRate);
      const int curStep = static_cast<int>(sinceRepeat / kRepeatRate);
      if (curStep > prevStep) {
        moveSelection(normDir);
      }
    }
  }
}

void UIEventSystem::moveSelection(const sf::Vector2f& direction) {
  if (m_selected == nullptr) {
    selectFirst();
    return;
  }

  if (!m_selected->isEnabled() || !m_selected->isInteractable()) {
    selectFirst();
    return;
  }

  UIWidget* target = nullptr;

  // Try explicit neighbour first, fall back to auto-find.
  const float ax = std::fabs(direction.x);
  const float ay = std::fabs(direction.y);

  if (ax > ay) {
    if (direction.x > 0.f) {
      target = m_selected->getNavRight();
    } 
    else {
      target = m_selected->getNavLeft();
    }
  } 
  else {
    if (direction.y > 0.f) {
      target = m_selected->getNavDown();
    } 
    else {
      target = m_selected->getNavUp();
    }
  }

  if (target == nullptr || !target->isEnabled() || !target->isInteractable()) {
    target = findSelectableInDirection(m_selected, direction);
  }

  if (target != nullptr) {
    setSelected(target);
  }
}

void UIEventSystem::selectFirst() {
  if (m_canvases.empty()) {
    return;
  }

  // Scan canvases from top to bottom; pick the first interactable widget.
  for (auto it = m_canvases.rbegin(); it != m_canvases.rend(); ++it) {
    for (auto& w : (*it)->m_widgets) {
      if (w->isEnabled() && w->isVisible() && w->isInteractable()) {
        setSelected(w);
        return;
      }
    }
  }
}

UIWidget* UIEventSystem::findSelectableInDirection(
  UIWidget* from, const sf::Vector2f& dir) const {
  if (from == nullptr || m_canvases.empty()) {
    return nullptr;
  }

  Canvas* canvas = from->getCanvas();
  if (canvas == nullptr) {
    return nullptr;
  }

  const sf::Vector2f origin = from->getRect().position +
    from->getRect().size * 0.5f;

  UIWidget* best = nullptr;
  float bestScore = -std::numeric_limits<float>::max();

  for (auto& w : canvas->m_widgets) {
    UIWidget* candidate = w;
    if (candidate == from) {
      continue;
    }
    if (!candidate->isEnabled() || !candidate->isVisible() ||
        !candidate->isInteractable()) {
      continue;
    }

    const sf::Vector2f cCenter = candidate->getRect().position +
      candidate->getRect().size * 0.5f;
    const sf::Vector2f offset = cCenter - origin;

    // Only consider candidates in the general direction
    const float dot = offset.x * dir.x + offset.y * dir.y;
    if (dot <= 0.f) {
      continue;
    }

    // Normalise offset length for angle scoring
    const float len = std::sqrt(offset.x * offset.x + offset.y * offset.y);
    if (len < 0.001f) {
      continue;
    }

    const float angleScore = dot / len;  // cos(angle) with desired direction
    const float distScore = 1.f / (1.f + len);  // closer = better

    // Weight: favour alignment more than distance
    const float score = angleScore * 3.f + distScore;

    if (score > bestScore) {
      bestScore = score;
      best = candidate;
    }
  }

  return best;
}

} // namespace sfmx
