/************************************************************************/
/**
 * @file UIImage.h
 * @author Swampertor
 * @date 2026/06/16
 * @brief  Widget that draws a textured sprite within its bounds.
 */
/************************************************************************/
#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

#include "scene/Component.h"
#include "ui/UIWidget.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

/** @brief How an image is fitted inside the widget rect */
enum class ImageFit : uint8
{
  kStretch,   //!< Fill widget bounds exactly (default)
  kContain,   //!< Fit inside, maintaining aspect ratio (letterbox)
  kCover,     //!< Fill widget, cropping overflow
};

/**
 * @brief Draws a textured sprite scaled to the widget's size.
 *
 * Supports sub-rect cropping, tint color, and three fit modes.
 */
class UIImage : public ComponentT<UIImage>, public UIWidget
{
 public:
  explicit UIImage(SceneNode* owner);

  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  /** @brief Set the texture (shared pointer keeps it alive) */
  void setTexture(SPtr<sf::Texture> tex);
  /** @brief Convenience: borrow a texture (caller must keep it alive) */
  void setTexture(const sf::Texture& tex);
  /** @brief Optional sub-rect within the texture (default = entire texture) */
  void setTextureRect(const sf::IntRect& rect);
  /** @brief Clear the sub-rect so the full texture is used */
  void clearTextureRect();

  /** @brief Tint colour (default = white / opaque) */
  FORCEINLINE void 
  setColor(sf::Color c)     { m_color = c; }
  NODISCARD FORCEINLINE sf::Color 
  getColor() const          { return m_color; }

  /** @brief How the image fits inside the widget rect */
  FORCEINLINE void 
  setFit(ImageFit fit)      { m_fit = fit; }
  NODISCARD FORCEINLINE ImageFit 
  getFit() const            { return m_fit; }

 private:
  SPtr<sf::Texture> m_texture;
  sf::IntRect       m_textureRect;
  bool              m_hasTexRect = false;
  sf::Color         m_color      = sf::Color::White;
  ImageFit          m_fit        = ImageFit::kStretch;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UIImage)
