/************************************************************************/
/**
 * @file UILabel.h
 * @author Swampertor
 * @date 2026/06/10
 * @brief  Non-interactive text label for the UI canvas.
 */
/************************************************************************/
#pragma once

#include <SFML/Graphics/Text.hpp>

#include "scene/SceneNode.h"
#include "scene/Component.h"
#include "ui/UIWidget.h"

namespace sfmx
{

class FontAsset;

/**
 * @brief A non-interactive text label that can live in a Canvas (standalone)
 *        or on a SceneNode (component mode, via ComponentT<UILabel>).
 *
 * Renders a single line of text via sf::Text.  The label does not respond
 * to pointer events — it is purely a visual element.
 *
 * Fonts are provided through the FontAsset system.  When no font is set the
 * label draws nothing.  Set a font via setFontAsset() before the widget is
 * drawn for the first time.
 *
 * Pool allocation caveat:
 *   This class allocates/deallocates in its constructor/destructor through the
 *   MemoryPool system (inherited from ComponentT).  Prefer creating labels at
 *   scene load and toggling enable/visible during gameplay rather than
 *   creating/destroying them at runtime.
 */
class UILabel final : public UIWidgetT<UILabel, WidgetType::kLabel>,
                      public ComponentT<UILabel>
{
 public:
  using UIWidget::isEnabled;
  using UIWidget::isVisible;
  using UIWidget::isInteractable;
  using UIWidget::setEnabled;
  using UIWidget::setVisible;
  using UIWidget::setInteractable;
  using UIWidget::setFocused;
  using UIWidget::getPosition;
  using UIWidget::setPosition;
  using UIWidget::getSize;
  using UIWidget::setSize;
  using UIWidget::getRect;
  using UIWidget::setRect;
  using UIWidget::getColor;
  using UIWidget::setColor;
  using UIWidget::containsPoint;
  using UIWidget::syncColliderToRect;

  // -- Constructors ----------------------------------------------------------

  /**
   * @brief Constructor for standalone (canvas) usage.
   * @param size  Initial size.
   */
  UILabel(sf::Vector2f size = {200.f, 50.f});

  /**
   * @brief Constructor for component usage (attached to a SceneNode).
   * @param node  The node this component belongs to.
   * @param size  Initial size.
   */
  UILabel(SceneNode* node,
          sf::Vector2f size = {200.f, 50.f});

  ~UILabel() override;

  /** @brief Type UUID for serialization. */
  NODISCARD UUID getTypeId() const override;

  // -- Text ------------------------------------------------------------------

  /** @brief Set the displayed string content. */
  FORCEINLINE void setText(StringView text) {
    if (m_text) { m_text->setString(String(text)); }
  }
  /** @brief Current displayed string content. */
  NODISCARD FORCEINLINE String getText() const {
    return m_text ? m_text->getString().toAnsiString() : String();
  }

  /** @brief Character size in points. */
  FORCEINLINE void setCharacterSize(uint32 size) {
    if (m_text) { m_text->setCharacterSize(size); }
  }
  /** @brief Current character size in points. */
  NODISCARD FORCEINLINE uint32 getCharacterSize() const {
    return m_text ? m_text->getCharacterSize() : 0;
  }

  /** @brief Fill colour of the rendered text. */
  FORCEINLINE void setTextColor(sf::Color color) {
    if (m_text) { m_text->setFillColor(color); }
  }
  /** @brief Current fill colour of the rendered text. */
  NODISCARD FORCEINLINE sf::Color getTextColor() const {
    return m_text ? m_text->getFillColor() : sf::Color::White;
  }

  // -- Font asset ------------------------------------------------------------

  /**
   * @brief  Set the font from a FontAsset pointer.
   *
   * If the asset is not yet loaded the method forces an asynchronous load via
   * the AssetManager.  The label will not draw until the font is available.
   */
  void setFontAsset(SPtr<FontAsset> asset);

  /**
   * @brief  Set the font by asset UUID (resolved via AssetManager).
   *
   * Useful when the FontAsset pointer is not available at call time (e.g.
   * during deserialization).
   */
  void setFontAssetId(const UUID& id);

  /** @brief  Currently assigned font asset, or nullptr. */
  NODISCARD SPtr<FontAsset> getFontAsset() const;
  /** @brief  UUID of the currently assigned font asset. */
  NODISCARD const UUID& getFontAssetId() const;

  // -- Serialization ---------------------------------------------------------

  /** @brief  Serialise text content, character size, and colour. */
  void onSerialize(DataStream& stream) const override;
  /** @brief  Restore state written by onSerialize. */
  void onDeserialize(DataStream& stream) override;

 private:
  /** @brief  Draw text at the widget position. */
  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  mutable UniquePtr<sf::Text> m_text;       ///< SFML text object (lazy-created on font set).
  UUID m_fontAssetId = UUID::null();        ///< Resolved font asset UUID.
  SPtr<FontAsset> m_fontAsset;              ///< Cached font asset pointer.
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UILabel)
