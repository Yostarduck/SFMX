#include "ui/UIVBox.h"

#include "scene/SceneNode.h"
#include "scene/Transform.h"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

namespace sfmx
{

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

UIVBox::UIVBox(SceneNode* owner)
  : ComponentT<UIVBox>(owner), UIWidget(owner)
{
}

// -----------------------------------------------------------------------------
// Layout
// -----------------------------------------------------------------------------

void
UIVBox::arrange() {
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

  float totalH = 0.f;
  for (auto& [n, w] : children)
    totalH += w->getSize().y;
  totalH += m_spacing * static_cast<float>(static_cast<int>(children.size()) - 1);

  const float leftover = std::max(0.f, usableH - totalH);
  float cy = m_padding + leftover * m_childAlign;

  for (auto& [node, w] : children) {
    const float childH = w->getSize().y;
    node->transform().setPosition({m_padding, cy});
    w->setSize({usableW, childH});
    cy += childH + m_spacing;
  }
}

// -----------------------------------------------------------------------------
// Update
// -----------------------------------------------------------------------------

void
UIVBox::onUpdate(float) {
  arrange();
}

// -----------------------------------------------------------------------------
// Draw
// -----------------------------------------------------------------------------

void
UIVBox::onDraw(sf::RenderTarget& target, sf::RenderStates states) const {
  if (m_size.x <= 0.f || m_size.y <= 0.f) return;

  sf::RectangleShape bg(m_size);
  bg.setFillColor(sf::Color::Transparent);
  bg.setOutlineColor({60, 60, 65});
  bg.setOutlineThickness(1.f);
  target.draw(bg, states);
}

} // namespace sfmx
