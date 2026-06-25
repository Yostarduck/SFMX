#pragma once

#include "scene/SceneNode.h"
#include "scene/Component.h"
#include "ui/UIWidget.h"

namespace sfmx
{

/**
 * @brief A clickable button that can live in a Canvas (standalone) or on a
 *        SceneNode (component mode, via ComponentT<UIButton>).
 *
 * Visual states:
 *   Normal, Hovered, Pressed, Disabled.
 *
 * The button draws a coloured rectangle (sf::RectangleShape) and transitions
 * its colour on state changes.
 *
 * Note on dual-mode:
 *   UIButton inherits from both UIWidget and ComponentT<UIButton>.  When used
 *   via addComponent<UIButton>(...), the ComponentT constructor attaches the
 *   button to the scene graph and pools it via MemoryPool; when created without
 *   a node (standalone constructor), only the UIWidget functionality is used.
 */
class UIButton final : public UIWidgetT<UIButton, WidgetType::kButton>, public ComponentT<UIButton>
{
 public:
  using UIWidget::isEnabled;
  using UIWidget::isVisible;
  using UIWidget::isInteractable;
  using UIWidget::setEnabled;
  using UIWidget::setVisible;
  using UIWidget::setInteractable;
  using UIWidget::setFocused;
  using UIWidget::getName;
  using UIWidget::setName;
  using UIWidget::getPosition;
  using UIWidget::setPosition;
  using UIWidget::getSize;
  using UIWidget::setSize;
  using UIWidget::getRect;
  using UIWidget::setRect;
  using UIWidget::getColor;
  using UIWidget::setColor;
  using UIWidget::getParent;
  using UIWidget::setParent;
  using UIWidget::containsPoint;
  using UIWidget::syncColliderToRect;
  using UIWidget::onPointerEnter;
  using UIWidget::onPointerExit;
  using UIWidget::onPointerDown;
  using UIWidget::onPointerUp;
  using UIWidget::onPointerClick;

  // -- Visual states ---------------------------------------------------------

  enum class VisualState
  {
    kNormal,
    kHovered,
    kPressed,
    kFocused,
    kDisabled
  };

  // -- Constructors ----------------------------------------------------------

  /**
   * @brief Constructor for standalone (canvas) usage.
   * @param name Display name.
   * @param size Initial size.
   */
  UIButton(sf::Vector2f size = {200.f, 50.f});

  /**
   * @brief Constructor for component usage (attached to a SceneNode).
   * @param node  The node this component belongs to.
   * @param name  Display name.
   * @param size  Initial size.
   */
  UIButton(SceneNode* node,
           sf::Vector2f size = {200.f, 50.f});

  ~UIButton() override;

  /** @brief Type UUID for serialization. */
  NODISCARD UUID getTypeId() const override;

  // -- Serialization -----------------------------------------------------------

  void onSerialize(DataStream& stream) const override;

  void onDeserialize(DataStream& stream) override;

  // -- State -----------------------------------------------------------------

  NODISCARD FORCEINLINE VisualState getVisualState() const { return m_visualState; }

  // -- Colour overrides ------------------------------------------------------

  FORCEINLINE void setNormalColor(sf::Color color) { m_normalColor = color; }
  FORCEINLINE void setHoveredColor(sf::Color color) { m_hoveredColor = color; }
  FORCEINLINE void setPressedColor(sf::Color color) { m_pressedColor = color; }
  FORCEINLINE void setDisabledColor(sf::Color color) { m_disabledColor = color; }
  FORCEINLINE void setFocusedColor(sf::Color color) { m_focusedColor = color; }

  NODISCARD FORCEINLINE sf::Color getNormalColor() const { return m_normalColor; }
  NODISCARD FORCEINLINE sf::Color getHoveredColor() const { return m_hoveredColor; }
  NODISCARD FORCEINLINE sf::Color getPressedColor() const { return m_pressedColor; }
  NODISCARD FORCEINLINE sf::Color getFocusedColor() const { return m_focusedColor; }
  NODISCARD FORCEINLINE sf::Color getDisabledColor() const { return m_disabledColor; }

 private:
  // -- UIWidget virtual overrides --------------------------------------------

  void onPointerEnter(sf::Vector2f position) override;
  void onPointerExit(sf::Vector2f position) override;
  void onPointerDown(sf::Vector2f position) override;
  void onPointerUp(sf::Vector2f position) override;

  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  sf::Color resolveColor() const;

  VisualState m_visualState = VisualState::kNormal;

  sf::Color m_normalColor   = sf::Color(200, 200, 200);
  sf::Color m_hoveredColor  = sf::Color(180, 180, 255);
  sf::Color m_pressedColor  = sf::Color(120, 120, 200);
  sf::Color m_focusedColor  = sf::Color(160, 160, 255);
  sf::Color m_disabledColor = sf::Color(100, 100, 100);
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UIButton)
