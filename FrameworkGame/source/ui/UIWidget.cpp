#include "ui/UIWidget.h"
#include "ui/Canvas.h"

namespace sfmx
{

UIWidget::UIWidget(StringView name)
    : m_name(name) {
}

UIWidget::~UIWidget() {
}

// -- Hierarchy ---------------------------------------------------------------

void UIWidget::setParent(UIWidget* parent) {
  if (m_parent == parent) {
    return;
  }

  // Remove from old parent
  if (m_parent != nullptr) {
    auto& siblings = m_parent->m_children;
    auto it = std::find(siblings.begin(), siblings.end(), this);
    if (it != siblings.end()) {
      siblings.erase(it);
    }
  }

  m_parent = parent;

  // Add to new parent
  if (parent != nullptr) {
    parent->m_children.push_back(this);
  }
}

UIWidget* UIWidget::getChild(size_t index) const {
  if (index >= m_children.size()) {
    return nullptr;
  }
  return m_children[index];
}

// -- Collider ----------------------------------------------------------------

void UIWidget::setColliderCircle(const sf::Vector2f& center, float radius) {
  m_collider = std::make_unique<CircleCollider>(center, radius);
}

void UIWidget::setColliderAABB(const sf::Vector2f& center, const sf::Vector2f& halfSize) {
  m_collider = std::make_unique<AABBCollider>(center, halfSize);
}

void UIWidget::setColliderOBB(const sf::Vector2f& center, const sf::Vector2f& halfSize) {
  m_collider = std::make_unique<OBBCollider>(center, halfSize);
}

void UIWidget::setColliderPoint(const sf::Vector2f& localPos) {
  m_collider = std::make_unique<PointCollider>(localPos);
}

void UIWidget::setColliderLine(const sf::Vector2f& localStart,
                               const sf::Vector2f& localEnd) {
  m_collider = std::make_unique<LineCollider>(localStart, localEnd);
}

void UIWidget::clearCollider() {
  m_collider.reset();
}

void UIWidget::syncColliderToRect() {
  const auto center = sf::Vector2f{m_rect.position.x + m_rect.size.x * 0.5f,
                                    m_rect.position.y + m_rect.size.y * 0.5f};
  const auto halfSize = m_rect.size * 0.5f;
  m_collider = std::make_unique<AABBCollider>(center, halfSize);
}

// -- Hit testing -------------------------------------------------------------

bool UIWidget::containsPoint(sf::Vector2f point) const {
  if (m_collider != nullptr) {
    // Use the physics dispatcher. Both transforms are identity because
    // the point is already in widget-local space.
    const auto result = intersect(*m_collider, sf::Transform::Identity,
                                  PointCollider(point), sf::Transform::Identity);
    return result.hit;
  }
  return m_rect.contains(point);
}

// -- Virtual event callbacks -------------------------------------------------

void UIWidget::onPointerEnter(sf::Vector2f position) {
  m_onPointerEnterEvent(position);
}

void UIWidget::onPointerExit(sf::Vector2f position) {
  m_onPointerExitEvent(position);
}

void UIWidget::onPointerDown(sf::Vector2f position) {
  m_onPointerDownEvent(position);
}

void UIWidget::onPointerUp(sf::Vector2f position) {
  m_onPointerUpEvent(position);
}

void UIWidget::onPointerClick(sf::Vector2f position) {
  m_onPointerClickEvent(position);
}

void UIWidget::onSelect() {
  m_onSelectEvent();
}

void UIWidget::onDeselect() {
  m_onDeselectEvent();
}

void UIWidget::onSubmit() {
  m_onSubmitEvent();
}

void UIWidget::onCancel() {
  m_onCancelEvent();
}

// -- Drawing -----------------------------------------------------------------

void UIWidget::onDraw(sf::RenderTarget& /*target*/,
                      sf::RenderStates /*states*/) const {
  // Base widget has no visual; subclasses override.
}

} // namespace sfmx
