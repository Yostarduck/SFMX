#include "ui/UIWidget.h"
#include "ui/Canvas.h"

namespace sfmx
{

bool UIWidget::s_canvasDrawing = false;

UIWidget::UIWidget() = default;

UIWidget::~UIWidget() {
  if (m_parent != nullptr) {
    m_parent->removeChild(this);
  }
  if (m_canvas != nullptr) {
    m_canvas->removeWidget(this);
  }
}

// -- Collider ----------------------------------------------------------------

void 
UIWidget::setColliderCircle(const sf::Vector2f& center, float radius) {
  m_collider = UniquePtr<ICollider>(new CircleCollider(center, radius));
}

void UIWidget::setColliderAABB(const sf::Vector2f& center, const sf::Vector2f& halfSize) {
  m_collider = UniquePtr<ICollider>(new AABBCollider(center, halfSize));
}

void UIWidget::setColliderOBB(const sf::Vector2f& center, const sf::Vector2f& halfSize) {
  m_collider = UniquePtr<ICollider>(new OBBCollider(center, halfSize));
}

void UIWidget::setColliderPoint(const sf::Vector2f& localPos) {
  m_collider = UniquePtr<ICollider>(new PointCollider(localPos));
}

void UIWidget::setColliderLine(const sf::Vector2f& localStart,
                               const sf::Vector2f& localEnd) {
  m_collider = UniquePtr<ICollider>(new LineCollider(localStart, localEnd));
}

void UIWidget::clearCollider() {
  m_collider.reset();
}

void UIWidget::syncColliderToRect() {
  const auto center = sf::Vector2f{m_rect.position.x + m_rect.size.x * 0.5f,
                                    m_rect.position.y + m_rect.size.y * 0.5f};
  const auto halfSize = m_rect.size * 0.5f;
  m_collider = UniquePtr<ICollider>(new AABBCollider(center, halfSize));
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
  if (!isEnabled() || !isVisible() || !isInteractable()) return nullptr;
  if (!containsPoint(point)) return nullptr;
  return isBlockingInput() ? const_cast<UIWidget*>(this) : nullptr;
}

sf::Vector2f UIWidget::toLocalSpace(sf::Vector2f canvasPoint) const {
  if (m_parent) {
    canvasPoint = m_parent->toLocalSpace(canvasPoint);
  }
  return canvasPoint - getPosition();
}

// -- Drawing -----------------------------------------------------------------

void UIWidget::onDraw(sf::RenderTarget& target,
                      sf::RenderStates states) const {
  SFMX_PARAMETER_UNUSED(target);
  SFMX_PARAMETER_UNUSED(states);
  // Base widget has no visual; subclasses override.
}

} // namespace sfmx
