#include "core/DataStream.h"
#include "ui/UIWidget.h"
#include "ui/UIManager.h"
#include "utils/MemoryPoolHandler.h"

namespace sfmx
{

namespace
{

/**
 * @brief Destroy a child widget through its memory pool (erased path — the
 *        concrete type is only reachable via getTypeId()). Mirrors how Scene
 *        destroys pooled components. Skipped when the pools are already gone
 *        (the widget then cannot have come from a pool either).
 */
void
destroyChild(UIWidget* child) {
  if (nullptr == child) {
    return;
  }
  if (MemoryPoolHandler::isStarted()) {
    MemoryPoolHandler::instance().deallocate(child->getTypeId(),
                                              static_cast<void*>(child));
  }
}

} // namespace

UIWidget::UIWidget() = default;

UIWidget::~UIWidget() {
  // 1. Drop any manager-side references while the manager is still reachable
  //    (clears selection / hover without touching members we still need).
  if (UIManager* manager = getManager()) {
    manager->forgetWidget(this);
  }

  // 2. Unlink from our parent (no-op for roots; a parent destroying us will
  //    have left m_parent intact so child dtors remove themselves here).
  if (m_parent != nullptr) {
    m_parent->removeChild(this);
  }

  // 3. Destroy our children (parent owns its children). Each child's dtor
  //    unlinks itself from m_children (step 2 above), so popping until empty
  //    is safe. If the pools are gone we can only detach them: clearing
  //    m_parent keeps a later child dtor from touching this dead parent.
  if (MemoryPoolHandler::isStarted()) {
    while (!m_children.empty()) {
      UIWidget* child = m_children.back();
      destroyChild(child);
      // A deallocate that did not destroy (child not from this pool) would
      // leave `child` at the back forever — detach it so we still make progress.
      if (!m_children.empty() && m_children.back() == child) {
        child->m_parent = nullptr;
        m_children.pop_back();
      }
    }
  } else {
    for (auto* child : m_children) {
      if (nullptr != child) {
        child->m_parent = nullptr;
      }
    }
    m_children.clear();
  }

  // 4. Last: unregister as a root (skipped when the UIManager is tearing its
  //    roots down — it nulls m_manager before destroying us).
  if (m_manager != nullptr) {
    m_manager->removeRoot(this);
  }
}

// -- Hit testing -------------------------------------------------------------

bool UIWidget::containsPoint(sf::Vector2f point) const {
  return m_rect.contains(point);
}

// -- Virtual event callbacks -------------------------------------------------

void UIWidget::triggerPointerEnter(sf::Vector2f position) {
  m_onPointerEnterEvent(position);
}

void UIWidget::triggerPointerExit(sf::Vector2f position) {
  m_onPointerExitEvent(position);
}

void UIWidget::triggerPointerDown(sf::Vector2f position) {
  m_onPointerDownEvent(position);
}

void UIWidget::triggerPointerUp(sf::Vector2f position) {
  m_onPointerUpEvent(position);
}

void UIWidget::triggerPointerClick(sf::Vector2f position) {
  m_onPointerClickEvent(position);
}

void UIWidget::triggerScroll(float delta) {
  SFMX_PARAMETER_UNUSED(delta);
}

void UIWidget::triggerSelect() {
  m_onSelectEvent();
}

void UIWidget::triggerDeselect() {
  m_onDeselectEvent();
}

void UIWidget::triggerSubmit() {
  m_onSubmitEvent();
}

void UIWidget::triggerCancel() {
  m_onCancelEvent();
}

// -- Hierarchy ----------------------------------------------------------------

void UIWidget::addChild(UIWidget* child) {
  if (nullptr == child || child == this) return;
  if (nullptr != child->m_parent) {
    child->m_parent->removeChild(child);
  }
  child->m_parent = this;
  m_children.push_back(child);
}

void UIWidget::removeChild(UIWidget* child) {
  if (nullptr == child || child->m_parent != this) return;
  for (size_t i = 0; i < m_children.size(); ++i) {
    if (m_children[i] == child) {
      m_children.erase(m_children.begin() + static_cast<ptrdiff_t>(i));
      child->m_parent = nullptr;
      return;
    }
  }
}

sf::Transform UIWidget::getChildTransform() const {
  return sf::Transform::Identity;
}

void UIWidget::drawHierarchy(sf::RenderTarget& target,
                             sf::RenderStates states) const {
  if (!isVisible()) return;
  onDraw(target, states);
  if (!m_children.empty()) {
    states.transform *= getChildTransform();
    for (auto* child : m_children) {
      child->drawHierarchy(target, states);
    }
  }
}

UIWidget* UIWidget::hitTestInHierarchy(sf::Vector2f point) const {
  // Invisible: transparent to input (no block, no receive).
  if (!isVisible()) return nullptr;
  if (!containsPoint(point)) return nullptr;

  // Dead (disabled / non-interactable) widgets still BLOCK: they swallow the
  // hit so widgets drawn underneath receive nothing, but the UIManager never
  // dispatches pointer events to them.
  if (!isEnabled() || !isInteractable()) {
    return const_cast<UIWidget*>(this);
  }

  // Recurse children topmost-first (reverse draw order), mapping the point
  // from THIS widget's frame into each child's frame with the INVERSE of
  // getChildTransform() — the exact reverse of the composition drawHierarchy
  // applies forward (child vertex → parent frame), so drawing and hit-testing
  // stay symmetric.
  const sf::Vector2f childPoint =
      getChildTransform().getInverse().transformPoint(point);
  for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
    if (UIWidget* hit = (*it)->hitTestInHierarchy(childPoint)) {
      return hit;
    }
  }

  // Inside us: return this regardless of blocksInput — the caller
  // (UIManager::hitTest) decides blocking vs. fall-through, so a non-blocking
  // widget no longer hides the widgets below it.
  return const_cast<UIWidget*>(this);
}

sf::Vector2f UIWidget::toLocalSpace(sf::Vector2f canvasPoint) const {
  if (m_parent) {
    canvasPoint = m_parent->toLocalSpace(canvasPoint);
  }
  return canvasPoint - getPosition();
}

// -- Lifecycle ----------------------------------------------------------------

void UIWidget::onUpdate(float deltaTime) {
  SFMX_PARAMETER_UNUSED(deltaTime);
  // Base widget has no per-frame behaviour; subclasses override.
}

// -- Drawing -----------------------------------------------------------------

void UIWidget::onDraw(sf::RenderTarget& target,
                      sf::RenderStates states) const {
  SFMX_PARAMETER_UNUSED(target);
  SFMX_PARAMETER_UNUSED(states);
  // Base widget has no visual; subclasses override.
}

// -- UI document: shared base payload ----------------------------------------

void
UIWidget::serializeBase(DataStream& stream) const {
  constexpr uint8 kBaseVersion = 1;
  stream << kBaseVersion;

  stream.writeString(getName());

  uint8 flags = 0;
  if (isEnabled())       flags |= 1 << 0;
  if (isVisible())       flags |= 1 << 1;
  if (isInteractable())  flags |= 1 << 2;
  if (isFocused())       flags |= 1 << 3;
  if (isBlockingInput()) flags |= 1 << 4;
  stream << flags;

  const sf::FloatRect& r = getRect();
  stream << r.position.x << r.position.y << r.size.x << r.size.y;

  const UISlot& s = getSlot();
  stream << s.anchorMin.x << s.anchorMin.y
         << s.anchorMax.x << s.anchorMax.y
         << s.pivot.x     << s.pivot.y
         << s.offset.x    << s.offset.y;

  const sf::Color& c = getColor();
  stream << c.r << c.g << c.b << c.a;
}

void
UIWidget::deserializeBase(DataStream& stream) {
  uint8 baseVersion = 0;
  stream >> baseVersion;
  if (baseVersion != 1) {
    return;
  }

  setName(stream.readString());

  uint8 flags = 0;
  stream >> flags;
  setEnabled((flags & (1 << 0)) != 0);
  setVisible((flags & (1 << 1)) != 0);
  setInteractable((flags & (1 << 2)) != 0);
  setFocused((flags & (1 << 3)) != 0);
  setBlocksInput((flags & (1 << 4)) != 0);

  sf::FloatRect r;
  stream >> r.position.x >> r.position.y >> r.size.x >> r.size.y;
  setRect(r);  // virtual — marks cached-geometry widgets dirty; syncs the
               // offset datum, which setOffset below then overwrites

  sf::Vector2f val;
  stream >> val.x >> val.y; setAnchorMin(val);
  stream >> val.x >> val.y; setAnchorMax(val);
  stream >> val.x >> val.y; setPivot(val);
  stream >> val.x >> val.y; setOffset(val);  // authoritative — runs last

  uint8 cr, cg, cb, ca;
  stream >> cr >> cg >> cb >> ca;
  setColor(sf::Color(cr, cg, cb, ca));
}

} // namespace sfmx
