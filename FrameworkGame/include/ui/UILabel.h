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

#include "ui/UIWidget.h"

namespace sfmx
{

class FontAsset;

/**
 * @brief A non-interactive text label in the UIManager-owned widget tree.
 *
 * Renders a single line of text via sf::Text.  The label does not respond
 * to pointer events — it is purely a visual element.
 *
 * Fonts are provided through the FontAsset system.  When no font is set the
 * label draws nothing.  Set a font via setFontAsset() before the widget is
 * drawn for the first time.
 *
 * Pool allocation caveat:
 *   Create labels with ui::createWidget<UILabel>(...) (memory pool) and
 *   prefer toggling enable/visible during gameplay rather than
 *   creating/destroying them at runtime.
 */
class UILabel final : public UIWidgetT<UILabel, WidgetType::kLabel>
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

  // -- Constructors ----------------------------------------------------------

  /**
   * @brief Constructor (normally called through ui::createWidget<UILabel>).
   * @param size  Initial size.
   */
  UILabel(sf::Vector2f size = {200.f, 50.f});

  ~UILabel() override;

  /** @brief Type UUID for serialization. */
  NODISCARD UUID getTypeId() const override;

  // -- Text ------------------------------------------------------------------

  /** @brief Set the displayed string content (interpreted as UTF-8). */
  FORCEINLINE void setText(StringView text) {
    m_textContent = sf::String::fromUtf8(text.begin(), text.end());
    if (m_text) { m_text->setString(m_textContent); }
  }
  /** @brief Current displayed string content, encoded as UTF-8. */
  NODISCARD FORCEINLINE String getText() const {
    const sf::U8String utf8 = m_textContent.toUtf8();
    return String(reinterpret_cast<const char*>(utf8.data()), utf8.size());
  }

  /** @brief Character size in points. */
  FORCEINLINE void setCharacterSize(uint32 size) {
    m_charSize = size;
    if (m_text) { m_text->setCharacterSize(size); }
  }
  /** @brief Current character size in points. */
  NODISCARD FORCEINLINE uint32 getCharacterSize() const { return m_charSize; }

  /** @brief Fill colour of the rendered text. */
  FORCEINLINE void setTextColor(sf::Color color) {
    m_textColor = color;
    if (m_text) { m_text->setFillColor(color); }
  }
  /** @brief Current fill colour of the rendered text. */
  NODISCARD FORCEINLINE sf::Color getTextColor() const { return m_textColor; }

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
  sf::String m_textContent;                 ///< UTF-32 backing store — survives a missing font.
  uint32 m_charSize = 30;                   ///< Backing character size (sf::Text default; applied on font set).
  sf::Color m_textColor = sf::Color::White; ///< Backing fill colour (applied on font set).
  UUID m_fontAssetId = UUID::null();        ///< Resolved font asset UUID.
  SPtr<FontAsset> m_fontAsset;              ///< Cached font asset pointer.
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UILabel)
