#include "ui/UIImage.h"

#include <SFML/Graphics/RenderTarget.hpp>

namespace sfmx
{

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

UIImage::UIImage(SceneNode* owner)
  : ComponentT<UIImage>(owner), UIWidget(owner)
{
}

// -----------------------------------------------------------------------------
// Texture
// -----------------------------------------------------------------------------

void
UIImage::setTexture(SPtr<sf::Texture> tex) {
  m_texture = std::move(tex);
}

void
UIImage::setTexture(const sf::Texture& tex) {
  m_texture = MakeShared<sf::Texture>(tex);
}

void
UIImage::setTextureRect(const sf::IntRect& rect) {
  m_textureRect = rect;
  m_hasTexRect  = true;
}

void
UIImage::clearTextureRect() {
  m_hasTexRect = false;
}

// -----------------------------------------------------------------------------
// Draw
// -----------------------------------------------------------------------------

void
UIImage::onDraw(sf::RenderTarget& target, sf::RenderStates states) const {
  if (!m_texture || m_size.x <= 0.f || m_size.y <= 0.f) return;

  sf::Sprite sprite(*m_texture);
  sprite.setColor(m_color);

  if (m_hasTexRect)
    sprite.setTextureRect(m_textureRect);

  const sf::Vector2f texSize = static_cast<sf::Vector2f>(m_hasTexRect
    ? sf::Vector2i(m_textureRect.size.x, m_textureRect.size.y)
    : sf::Vector2i(m_texture->getSize()));

  if (texSize.x <= 0.f || texSize.y <= 0.f) return;

  const float sx = m_size.x / texSize.x;
  const float sy = m_size.y / texSize.y;

  switch (m_fit) {
    case ImageFit::kContain: {
      const float s = std::min(sx, sy);
      sprite.setScale({s, s});
      sprite.setOrigin({texSize.x * 0.5f, texSize.y * 0.5f});
      sprite.setPosition(m_size * 0.5f);
      break;
    }
    case ImageFit::kCover: {
      const float s = std::max(sx, sy);
      sprite.setScale({s, s});
      sprite.setOrigin({texSize.x * 0.5f, texSize.y * 0.5f});
      sprite.setPosition(m_size * 0.5f);
      break;
    }
    case ImageFit::kStretch:
    default:
      sprite.setScale({sx, sy});
      break;
  }

  target.draw(sprite, states);
}

} // namespace sfmx
