#include "ui/UIHBox.h"

#include "scene/SceneNode.h"
#include "scene/Transform.h"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

namespace sfmx
{

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

UIHBox::UIHBox(SceneNode* owner)
  : ComponentT<UIHBox>(owner), UIWidget(owner)
{
}

// -----------------------------------------------------------------------------
// Layout
// -----------------------------------------------------------------------------

void
UIHBox::arrange() {
  SceneNode* own = UIWidget::m_owner;

  Vector<std::pair<SceneNode*, UIWidget*>> children;
  for (SceneNode* c = own->getFirstChild(); c; c = c->getNextSibling()) {
    for (Component* comp = c->getFirstComponent(); comp; comp = comp->getNextComponent()) {
      UIWidget* w = dynamic_cast<UIWidget*>(comp);
      if (w) { children.emplace_back(c, w); break; }
    }
  }
  if (children.empty()) return;

  const float usableW = m_size.x - m_padding * 2.f;
  const float usableH = m_size.y - m_padding * 2.f;

  float totalW = 0.f;
  for (auto& [n, w] : children)
    totalW += w->getSize().x;
  totalW += m_spacing * static_cast<float>(static_cast<int>(children.size()) - 1);

  const float leftover = std::max(0.f, usableW - totalW);
  float cx = m_padding + leftover * m_childAlign;

  for (auto& [node, w] : children) {
    const float childW = w->getSize().x;
    node->transform().setPosition({cx, m_padding});
    w->setSize({childW, usableH});
    cx += childW + m_spacing;
  }
}

// -----------------------------------------------------------------------------
// Update
// -----------------------------------------------------------------------------

void
UIHBox::onUpdate(float) {
  arrange();
}

// -----------------------------------------------------------------------------
// Draw
// -----------------------------------------------------------------------------

void
UIHBox::onDraw(sf::RenderTarget& target, sf::RenderStates states) const {
  if (m_size.x <= 0.f || m_size.y <= 0.f) return;

  sf::RectangleShape bg(m_size);
  bg.setFillColor(sf::Color::Transparent);
  bg.setOutlineColor({60, 60, 65});
  bg.setOutlineThickness(1.f);
  target.draw(bg, states);
}

} // namespace sfmx
