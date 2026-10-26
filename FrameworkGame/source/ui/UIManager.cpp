#include "ui/UIManager.h"

#include "input/InputAction.h"
#include "input/Mouse.h"
#include "utils/MemoryPoolHandler.h"

#include "ui/UIButton.h"
#include "ui/UICheckbox.h"
#include "ui/UIHorizontalBox.h"
#include "ui/UIImage.h"
#include "ui/UILabel.h"
#include "ui/UIScrollView.h"
#include "ui/UISlider.h"
#include "ui/UITextBox.h"
#include "ui/UIVerticalBox.h"
#include "ui/UISerializer.h"
#include "ui/UIWidgetRegistry.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

namespace sfmx
{

namespace
{

/** @brief Recursively find the first widget named @p name (DFS preorder). */
UIWidget*
findNamed(UIWidget* widget, StringView name) {
  if (nullptr == widget) {
    return nullptr;
  }
  if (widget->getName() == name) {
    return widget;
  }
  for (auto* child : widget->getChildren()) {
    if (UIWidget* found = findNamed(child, name)) {
      return found;
    }
  }
  return nullptr;
}

/**
 * @brief Map a point from @p w's rect frame to root (window-pixel) space.
 *
 * frame(child) = frame(parent) o parent->getChildTransform(): a point in the
 * child's rect frame becomes a parent-frame point by applying the parent's
 * getChildTransform — the same composition drawHierarchy bakes into its
 * render states, one parent at a time up to the root. Keeps navigation
 * scoring correct for nested / scrolled widgets.
 */
sf::Vector2f
toRootSpace(const UIWidget* w, sf::Vector2f point) {
  const UIWidget* child = w;
  while (nullptr != child && nullptr != child->getUIparent()) {
    const UIWidget* parent = child->getUIparent();
    point = parent->getChildTransform().transformPoint(point);
    child = parent;
  }
  return point;
}

/**
 * @brief First selectable widget in DFS preorder (roots in insertion order).
 *
 * Containers (boxes / scroll views) are layout constructs, not controls —
 * they are never navigation candidates, so they stay excluded here.
 */
UIWidget*
findFirstSelectable(UIWidget* widget) {
  if (nullptr == widget || !widget->isEnabled() || !widget->isVisible()) {
    return nullptr;
  }
  const WidgetType type = widget->getType();
  const bool isContainer =
    type == WidgetType::kVerticalBox ||
    type == WidgetType::kHorizontalBox ||
    type == WidgetType::kScrollView;
  if (!isContainer && widget->isInteractable()) {
    return widget;
  }
  for (auto* child : widget->getChildren()) {
    if (UIWidget* found = findFirstSelectable(child)) {
      return found;
    }
  }
  return nullptr;
}

/**
 * @brief Collect every navigation candidate in DFS preorder into @p out.
 *        Same rules as findFirstSelectable (visible, enabled, interactable,
 *        non-container).
 */
void
collectSelectables(UIWidget* widget, Vector<UIWidget*>& out) {
  if (nullptr == widget || !widget->isEnabled() || !widget->isVisible()) {
    return;
  }
  const WidgetType type = widget->getType();
  const bool isContainer =
    type == WidgetType::kVerticalBox ||
    type == WidgetType::kHorizontalBox ||
    type == WidgetType::kScrollView;
  if (!isContainer && widget->isInteractable()) {
    out.push_back(widget);
  }
  for (auto* child : widget->getChildren()) {
    collectSelectables(child, out);
  }
}

/**
 * @brief Apply a root widget's slot against @p viewport.
 *
 * setPosition rewrites the slot offset (it is the raw-placement datum), so
 * the authoritative anchor-relative offset is captured first and restored
 * after the computed position has been applied.
 */
void
applySlot(UIWidget* widget, const sf::Vector2f& viewport) {
  const sf::Vector2f offset = widget->getOffset();
  widget->setPosition(
    widget->getSlot().computePosition(viewport, widget->getSize()));
  widget->setOffset(offset);
}

} // namespace

// -- Lifecycle ---------------------------------------------------------------

void UIManager::onStartUp() {
  // Widget pools live here now (moved out of Game/DemoScene.cpp): registration
  // must precede the first ui::createWidget<T> allocation. hasPool-guarded so
  // a restart over pre-existing registrations doesn't assert.
  MemoryPoolHandler& pools = MemoryPoolHandler::instance();
  if (!pools.hasPool<UIButton>())        { pools.registerPool<UIButton>(64); }
  if (!pools.hasPool<UILabel>())         { pools.registerPool<UILabel>(64); }
  if (!pools.hasPool<UIImage>())         { pools.registerPool<UIImage>(64); }
  if (!pools.hasPool<UICheckbox>())      { pools.registerPool<UICheckbox>(64); }
  if (!pools.hasPool<UITextBox>())       { pools.registerPool<UITextBox>(64); }
  if (!pools.hasPool<UISlider>())        { pools.registerPool<UISlider>(64); }
  if (!pools.hasPool<UIVerticalBox>())   { pools.registerPool<UIVerticalBox>(16); }
  if (!pools.hasPool<UIHorizontalBox>()) { pools.registerPool<UIHorizontalBox>(16); }
  if (!pools.hasPool<UIScrollView>())    { pools.registerPool<UIScrollView>(16); }

  // Widget factories for UI documents (UISerializer): registered together
  // with the pools so the UI layer stays self-contained — create() allocates
  // from the very pools registered above. registerWidget overwrites by type
  // id, so re-registration on a restart is harmless.
  if (!UIWidgetRegistry::isStarted()) {
    UIWidgetRegistry::startUp();
  }
  UIWidgetRegistry& registry = UIWidgetRegistry::instance();
  registry.registerWidget<UIButton>();
  registry.registerWidget<UILabel>();
  registry.registerWidget<UIImage>();
  registry.registerWidget<UICheckbox>();
  registry.registerWidget<UITextBox>();
  registry.registerWidget<UISlider>();
  registry.registerWidget<UIVerticalBox>();
  registry.registerWidget<UIHorizontalBox>();
  registry.registerWidget<UIScrollView>();

  m_roots.clear();
  m_selected = nullptr;
  m_pointer = PointerState{};
  m_navigateAction = nullptr;
  m_submitAction = nullptr;
  m_cancelAction = nullptr;
  m_navTimer = 0.f;
  m_navHeld = false;
}

void UIManager::onShutDown() {
  // Drop state without firing events — script listeners are already gone
  // by teardown time.
  m_selected = nullptr;
  m_navigateAction = nullptr;
  m_submitAction = nullptr;
  m_cancelAction = nullptr;
  m_submitSub = {};
  m_cancelSub = {};
  m_pointer = PointerState{};

  // Destroy the roots we own: detach each from the manager first so its dtor
  // doesn't re-enter removeRoot while we iterate, then return it to its pool.
  const Vector<UIWidget*> roots = std::move(m_roots);
  m_roots.clear();
  for (auto* root : roots) {
    if (nullptr == root) {
      continue;
    }
    root->m_manager = nullptr;
    if (MemoryPoolHandler::isStarted()) {
      MemoryPoolHandler::instance().deallocate(root->getTypeId(),
                                                static_cast<void*>(root));
    }
  }
}

void UIManager::update(const sf::WindowBase& window, float deltaTime) {
  validateSelection();
  processPointer(window);
  processScroll();
  processNavigation(deltaTime);

  // Drive widget updates (layout refresh, slider drag, ...) for each root.
  // Unconditional (no m_enabled gate) and after the input phases above, so
  // widgets react to the state just refreshed.
  for (auto* root : m_roots) {
    updateHierarchy(root, deltaTime);
  }
}

void UIManager::updateHierarchy(UIWidget* widget, float deltaTime) {
  if (nullptr == widget) {
    return;
  }
  widget->onUpdate(deltaTime);
  for (auto* child : widget->getChildren()) {
    updateHierarchy(child, deltaTime);
  }
}

void UIManager::draw(sf::RenderTarget& target) {
  // Screen-space UI in the CURRENT window pixels: getDefaultView() is built
  // once at construction and never tracks resizes (SFML's onResize only
  // re-applies the current view), while hit-testing uses raw window pixels —
  // both spaces must agree or cursor and pixels drift apart after a resize.
  const sf::View prevView = target.getView();
  const sf::Vector2u size = target.getSize();
  target.setView(
    sf::View(sf::Vector2f({size.x * 0.5f, size.y * 0.5f}),
             sf::Vector2f({static_cast<float>(size.x),
                           static_cast<float>(size.y)})));

  sf::RenderStates states = sf::RenderStates::Default;
  for (auto* root : m_roots) {
    if (nullptr != root) {
      root->drawHierarchy(target, states);
    }
  }

  target.setView(prevView);
}

// -- Event routing -----------------------------------------------------------

UITextBox* UIManager::activeTextEditor() const {
  if (nullptr == m_selected || !m_selected->isEnabled() ||
      !m_selected->isVisible() || m_selected->getManager() == nullptr) {
    return nullptr;
  }
  return dynamic_cast<UITextBox*>(m_selected);
}

bool UIManager::handleEvent(const sf::Event& event) {
  // Viewport change → re-apply every root slot (the implicit viewport root
  // panel). Not consumed: the game still needs resize events.
  if (event.is<sf::Event::Resized>()) {
    const auto* resized = event.getIf<sf::Event::Resized>();
    relayout(sf::Vector2f(resized->size));
    return false;
  }

  UITextBox* editor = activeTextEditor();

  if (event.is<sf::Event::TextEntered>()) {
    if (nullptr == editor) {
      return false;
    }
    const char32_t ch = event.getIf<sf::Event::TextEntered>()->unicode;
    if (ch == 8) {
      editor->deleteCharacter();  // backspace arrives as text code-point 8
    } else if (ch >= 32) {
      editor->insertCharacter(static_cast<uint32>(ch));
    }
    // Everything else (Enter, Escape's 27, ...) is dropped: an editor owns
    // the keyboard while it has focus.
    return true;
  }

  if (event.is<sf::Event::KeyPressed>()) {
    if (nullptr == editor) {
      return false;
    }
    using K = sf::Keyboard::Key;
    switch (event.getIf<sf::Event::KeyPressed>()->code) {
      case K::Left:   editor->moveCursorLeft();  return true;
      case K::Right:  editor->moveCursorRight(); return true;
      case K::Home:   editor->moveCursorHome();  return true;
      case K::End:    editor->moveCursorEnd();   return true;
      case K::Delete: editor->deleteForward();   return true;
      // Backspace was already applied via TextEntered(8) — consume the key
      // as well so gameplay never sees it, but don't delete twice.
      case K::Backspace:                        return true;
      // Escape leaves the textbox; without a focused editor it falls through
      // to the game, where it still closes the window.
      case K::Escape:
        m_selected->triggerCancel();  // UITextBox::triggerCancel unfocuses
        return true;
      default:                                  return false;
    }
  }

  return false;
}

// -- Root registry -----------------------------------------------------------

void UIManager::relayout(const sf::Vector2f& viewportSize) {
  m_viewportSize = viewportSize;
  for (auto* root : m_roots) {
    if (nullptr != root) {
      applySlot(root, viewportSize);
    }
  }
}

bool UIManager::saveUI(const String& path) {
  return UISerializer::saveToFile(m_roots, FileSystemPath(path));
}

bool UIManager::loadUI(const String& path) {
  Vector<UIWidget*> roots;
  if (!UISerializer::loadFromFile(roots, FileSystemPath(path))) {
    return false;
  }
  for (UIWidget* root : roots) {
    addRoot(root);
  }
  return true;
}

void UIManager::addRoot(UIWidget* widget) {
  if (nullptr == widget) {
    return;
  }
  if (nullptr != widget->m_parent) {
    widget->m_parent->removeChild(widget);
  }
  if (widget->m_manager == this) {
    return;
  }
  widget->m_manager = this;
  m_roots.push_back(widget);

  // Apply the widget's slot right away while the viewport is known ("anchors
  // applied on add"); before the first Resized the authored position stands —
  // with default anchors that IS the slot position, so nothing jumps.
  if (m_viewportSize.x > 0.f && m_viewportSize.y > 0.f) {
    applySlot(widget, m_viewportSize);
  }
}

bool UIManager::removeRoot(UIWidget* widget) {
  for (auto it = m_roots.begin(); it != m_roots.end(); ++it) {
    if (*it == widget) {
      m_roots.erase(it);
      widget->m_manager = nullptr;
      return true;
    }
  }
  return false;
}

UIWidget* UIManager::findByName(StringView name) const {
  for (auto* root : m_roots) {
    if (UIWidget* found = findNamed(root, name)) {
      return found;
    }
  }
  return nullptr;
}

// -- Selection ---------------------------------------------------------------

void UIManager::setSelected(UIWidget* widget) {
  if (m_selected == widget) {
    return;
  }

  if (m_selected != nullptr) {
    m_selected->setFocused(false);
    m_selected->triggerDeselect();
  }

  m_selected = widget;

  if (m_selected != nullptr) {
    m_selected->setFocused(true);
    m_selected->triggerSelect();
  }
}

void UIManager::forgetWidget(UIWidget* widget) {
  if (nullptr == widget) {
    return;
  }
  if (m_selected == widget) {
    setSelected(nullptr);  // fires onDeselect while the widget is still intact
  }
  // Hover/press are dropped silently: firing an exit/up event into a widget
  // that is mid-destruction would run callbacks against dying members (and the
  // pool slot may already be recycled next frame).
  if (m_pointer.hovered == widget) {
    m_pointer.hovered = nullptr;
  }
  if (m_pointer.pressed == widget) {
    m_pointer.pressed = nullptr;
  }
}

// -- InputAction integration -------------------------------------------------

void UIManager::setSubmitAction(InputAction* action) {
  m_submitAction = action;
  m_submitSub = {};  // disconnect previous

  if (action != nullptr) {
    m_submitSub = action->onPerformed([this](const InputContext& ctx) {
      SFMX_PARAMETER_UNUSED(ctx);
      if (m_selected != nullptr) {
        m_selected->triggerSubmit();
      }
    });
  }
}

void UIManager::setCancelAction(InputAction* action) {
  m_cancelAction = action;
  m_cancelSub = {};  // disconnect previous

  if (action != nullptr) {
    m_cancelSub = action->onPerformed([this](const InputContext& ctx) {
      SFMX_PARAMETER_UNUSED(ctx);
      if (m_selected != nullptr) {
        m_selected->triggerCancel();
      }
    });
  }
}

// -- Internal ----------------------------------------------------------------

void UIManager::validateSelection() {
  if (m_selected == nullptr) {
    return;
  }

  if (!m_selected->isEnabled() || !m_selected->isVisible() ||
      m_selected->getManager() == nullptr) {
    setSelected(nullptr);
  }
}

UIWidget* UIManager::hitTest(sf::Vector2f point) const {
  UIWidget* fallback = nullptr;

  // Iterate roots in reverse (back = drawn last = topmost): stop at the first
  // blocking hit; non-blocking hits become the fallback and the search
  // continues — hitTestInHierarchy reports non-blocking and dead widgets so
  // neither can hide what is drawn underneath them.
  for (auto it = m_roots.rbegin(); it != m_roots.rend(); ++it) {
    UIWidget* w = *it;
    if (nullptr == w) continue;
    UIWidget* hit = w->hitTestInHierarchy(point);
    if (nullptr != hit) {
      if (hit->isBlockingInput()) {
        return hit;
      }
      if (nullptr == fallback) {
        fallback = hit;
      }
    }
  }
  return fallback;
}

void UIManager::processPointer(const sf::WindowBase& window) {
  const sf::Vector2i screenPos = sf::Mouse::getPosition(window);

  if (m_roots.empty()) {
    m_pointer.screenPos = screenPos;
    m_pointer.hovered = nullptr;
    m_pointer.canvasPos = {};
    return;
  }

  // No canvas transform anymore: root space IS window-pixel space.
  const sf::Vector2f canvasPos = static_cast<sf::Vector2f>(screenPos);

  m_pointer.screenPos = screenPos;
  m_pointer.canvasPos = canvasPos;

  UIWidget* rawHit = hitTest(canvasPos);

  // Dead hits (disabled / non-interactable) are still returned by hitTest so
  // they block widgets drawn underneath — but they never receive pointer
  // events themselves, and neither does anything below them.
  UIWidget* hit = nullptr;
  if (nullptr != rawHit && rawHit->isEnabled() && rawHit->isVisible() &&
      rawHit->isInteractable()) {
    hit = rawHit;
  }

  // Convert root-space point to each widget's local space before dispatching.
  const sf::Vector2f localPos   = (nullptr != hit)   ? hit->toLocalSpace(canvasPos)   : canvasPos;
  const sf::Vector2f localPosHovered =
    (nullptr != m_pointer.hovered) ? m_pointer.hovered->toLocalSpace(canvasPos) : canvasPos;

  // -- Enter / Exit --------------------------------------------------------
  if (hit != m_pointer.hovered) {
    if (m_pointer.hovered != nullptr) {
      m_pointer.hovered->triggerPointerExit(localPosHovered);
    }

    m_pointer.hovered = hit;

    if (hit != nullptr) {
      hit->triggerPointerEnter(localPos);
    }
  }

  // -- Button state --------------------------------------------------------
  const bool isDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);

  if (isDown && !m_pointer.buttonDown) {
    m_pointer.buttonDown = true;
    m_pointer.pressed = hit;

    if (hit != nullptr) {
      hit->triggerPointerDown(localPos);
      setSelected(hit);
    } else {
      setSelected(nullptr);
    }
  } 
  else if (!isDown && m_pointer.buttonDown) {
    m_pointer.buttonDown = false;

    if (m_pointer.pressed != nullptr) {
      const sf::Vector2f localPosPressed =
        m_pointer.pressed->toLocalSpace(canvasPos);
      m_pointer.pressed->triggerPointerUp(localPosPressed);

      if (m_pointer.pressed == hit && hit != nullptr) {
        hit->triggerPointerClick(localPos);
      }
    }

    m_pointer.pressed = nullptr;
  }
}

void UIManager::processScroll() {
  const float wheelDelta = Mouse::instance().getWheelDelta();
  if (std::fabs(wheelDelta) < 0.001f) return;

  // Walk up from the hovered widget looking for a scroll container
  for (UIWidget* w = m_pointer.hovered; w != nullptr; w = w->getUIparent()) {
    if (w->isEnabled() && w->isVisible() &&
        w->getType() == WidgetType::kScrollView) {
      w->triggerScroll(wheelDelta);
      return;
    }
  }
}

void UIManager::processNavigation(float deltaTime) {
  if (m_navigateAction == nullptr) {
    return;
  }

  if (m_selected != nullptr && m_selected->isTextEditor()) {
    return;
  }

  if (m_navigateAction->getValueType() != ActionValueType::kAxis2D) {
    std::cerr << "UIManager: navigate action \""
              << m_navigateAction->getName()
              << "\" must be an Axis2D input" << std::endl;
    return;
  }

  const InputValue& navValue = m_navigateAction->getValue();
  const sf::Vector2f dir = navValue.asVector2();

  const float mag = std::sqrt(dir.x * dir.x + dir.y * dir.y);
  if (mag < 0.001f) {
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

void UIManager::moveSelection(const sf::Vector2f& direction) {
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

void UIManager::selectFirst() {
  // First navigation candidate in DFS preorder over every root — widgets
  // nested inside boxes/scroll views are now reachable (was root-only).
  for (auto* root : m_roots) {
    if (UIWidget* first = findFirstSelectable(root)) {
      setSelected(first);
      return;
    }
  }
}

UIWidget* UIManager::findSelectableInDirection(
  UIWidget* from, const sf::Vector2f& dir) const {
  if (from == nullptr || m_roots.empty()) {
    return nullptr;
  }

  // Origin: root-space centre of the widget we navigate from — must be
  // transformed, since a nested getRect() is parent-local.
  const sf::Vector2f origin =
    toRootSpace(from, from->getRect().position + from->getRect().size * 0.5f);

  // Every navigation candidate in the tree (visible, enabled, interactable,
  // non-container) — nested widgets are reachable; ties keep DFS order.
  Vector<UIWidget*> candidates;
  for (auto* root : m_roots) {
    collectSelectables(root, candidates);
  }

  UIWidget* best = nullptr;
  float bestScore = -std::numeric_limits<float>::max();

  for (UIWidget* candidate : candidates) {
    if (candidate == from) {
      continue;
    }

    // Root-space centre of the candidate (nested / scrolled aware).
    const sf::Vector2f cCenter = toRootSpace(
      candidate,
      candidate->getRect().position + candidate->getRect().size * 0.5f);
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
