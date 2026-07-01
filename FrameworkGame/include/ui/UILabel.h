#pragma once

#include <SFML/Graphics/Text.hpp>

#include "scene/SceneNode.h"
#include "scene/Component.h"
#include "ui/UIWidget.h"

namespace sfmx
{

/**
 * @brief A non-interactive text label for the UI canvas.
 *
 * Follows the same dual-mode pattern as UIButton: can live standalone in a
 * Canvas or on a SceneNode via ComponentT<UILabel>.
 */
class UILabel final : public UIWidgetT<UILabel, WidgetType::kLabel>, public ComponentT<UILabel>
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

  UILabel(sf::Vector2f size = {200.f, 50.f});

  UILabel(SceneNode* node,
          sf::Vector2f size = {200.f, 50.f});

  ~UILabel() override;

  NODISCARD UUID getTypeId() const override;

  // -- Text ------------------------------------------------------------------

  FORCEINLINE void setText(StringView text) {
    if (m_text) { m_text->setString(String(text)); }
  }
  NODISCARD FORCEINLINE String getText() const {
    return m_text ? m_text->getString().toAnsiString() : String();
  }

  void setFont(SPtr<sf::Font> font);
  NODISCARD FORCEINLINE SPtr<sf::Font> getFont() const { return m_font; }

  FORCEINLINE void setCharacterSize(uint32 size) {
    if (m_text) { m_text->setCharacterSize(size); }
  }
  NODISCARD FORCEINLINE uint32 getCharacterSize() const {
    return m_text ? m_text->getCharacterSize() : 0;
  }

  FORCEINLINE void setTextColor(sf::Color color) {
    if (m_text) { m_text->setFillColor(color); }
  }
  NODISCARD FORCEINLINE sf::Color getTextColor() const {
    return m_text ? m_text->getFillColor() : sf::Color::White;
  }

  // -- Serialization ---------------------------------------------------------

  void onSerialize(DataStream& stream) const override;
  void onDeserialize(DataStream& stream) override;

 private:
  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  mutable UniquePtr<sf::Text> m_text;
  // TODO: Create later a FontAsset class for fonts
  SPtr<sf::Font> m_font;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UILabel)
