#include "scene/InstancedSpriteComponent.h"

#include <SFML/Graphics/RenderTarget.hpp>

#include "assets/TextureAsset.h"
#include "scene/SceneNode.h"

namespace sfmx
{

InstancedSpriteComponent::InstancedSpriteComponent(SceneNode* owner)
  : ComponentT<InstancedSpriteComponent>(owner)
{}

void
InstancedSpriteComponent::setAtlas(SPtr<TextureAsset> atlas,
                                   const Vector<sf::IntRect>& frames,
                                   const sf::BlendMode& blend,
                                   size_t reserveInstances) {
  m_atlasAsset = std::move(atlas);
  if (nullptr == m_atlasAsset || !InstanceDrawer::isStarted()) {
    return;
  }

  const sf::Texture& texture = m_atlasAsset->texture();
  m_atlas = InstanceDrawer::instance().registerAtlas(
      &texture, blend, frames, texture.getSize(), reserveInstances);
}

void
InstancedSpriteComponent::setTexture(SPtr<TextureAsset> texture,
                                     const sf::BlendMode& blend,
                                     size_t reserveInstances) {
  if (nullptr == texture) {
    m_atlasAsset = nullptr;
    return;
  }

  const sf::Vector2u size = texture->texture().getSize();
  const Vector<sf::IntRect> frame = {sf::IntRect(
      {0, 0}, {static_cast<int>(size.x), static_cast<int>(size.y)})};
  setAtlas(std::move(texture), frame, blend, reserveInstances);
}

void
InstancedSpriteComponent::onDraw(sf::RenderTarget& target,
                                 sf::RenderStates states) const {
  SFMX_PARAMETER_UNUSED(target);
  SFMX_PARAMETER_UNUSED(states);

  if (!m_atlas.isValid() || !InstanceDrawer::isStarted()) {
    return;
  }

  const sf::Vector2f center =
      m_owner->getWorldTransform().transformPoint({0.f, 0.f});
  InstanceDrawer::instance().submit(m_atlas, center, m_frame, m_size,
                                    m_rotation, m_tint);
}

} // namespace sfmx
