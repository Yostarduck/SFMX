#pragma once

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

#include "core/platform/Prerequisites.h"
#include "gfx/InstanceDrawer.h"
#include "scene/Component.h"

namespace sfmx
{

class TextureAsset;

/**
 * @brief A sprite drawn through the shared @ref InstanceDrawer instead of its
 *        own draw call.
 *
 * Many of these that share an atlas collapse into a single instanced GPU draw.
 * The component itself is deliberately trivial (an atlas handle, a frame index,
 * a size, a rotation and a tint) so it can spawn and die weightlessly from a
 * pool; it owns no per-frame scratch. Its draw only enqueues one instance into
 * the drawer, which the scene flushes after the traversal.
 */
class InstancedSpriteComponent : public ComponentT<InstancedSpriteComponent>
{
 public:
  explicit InstancedSpriteComponent(SceneNode* owner);

  /**
   * @brief Bind this sprite to an atlas and register the batch bucket for it.
   *
   * @param atlas            Texture asset kept alive for as long as this draws.
   * @param frames           Sub-rects in pixels; @ref setFrame selects one.
   * @param blend            Blend mode for the shared batch.
   * @param reserveInstances Buffer capacity reserved for this atlas's batch.
   */
  void
  setAtlas(SPtr<TextureAsset> atlas,
           const Vector<sf::IntRect>& frames,
           const sf::BlendMode& blend = sf::BlendAlpha,
           size_t reserveInstances = 1024);

  void
  setFrame(uint32 frame) { m_frame = frame; }

  void
  setSize(const sf::Vector2f& size) { m_size = size; }

  void
  setRotation(float radians) { m_rotation = radians; }

  void
  setTint(const sf::Color& tint) { m_tint = tint; }

  NODISCARD FORCEINLINE uint32
  getFrame() const { return m_frame; }

  NODISCARD FORCEINLINE const sf::Vector2f&
  getSize() const { return m_size; }

  NODISCARD FORCEINLINE float
  getRotation() const { return m_rotation; }

  NODISCARD FORCEINLINE const sf::Color&
  getTint() const { return m_tint; }

  void
  onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

 private:
  SPtr<TextureAsset> m_atlasAsset;
  AtlasHandle        m_atlas;
  sf::Vector2f       m_size{0.f, 0.f};
  float              m_rotation = 0.f;
  uint32             m_frame = 0u;
  sf::Color          m_tint = sf::Color::White;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::InstancedSpriteComponent)
