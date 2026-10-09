#include "ui/UIButton.h"
#include "core/DataStream.h"

namespace sfmx
{

UIButton::UIButton(sf::Vector2f size)
  : UIWidgetT<UIButton, WidgetType::kButton>() {
  setSize(size);
}

UIButton::~UIButton() = default;

void UIButton::setSize(sf::Vector2f size) {
  UIWidget::setSize(size);
  m_visualDirty = true;
}

void UIButton::setPosition(sf::Vector2f position) {
  UIWidget::setPosition(position);
  m_visualDirty = true;
}

void UIButton::setRect(const sf::FloatRect& rect) {
  UIWidget::setRect(rect);
  m_visualDirty = true;
}

void UIButton::setEnabled(bool enabled) {
  UIWidget::setEnabled(enabled);
  m_visualDirty = true;
}

// -- Type --------------------------------------------------------------------

UUID UIButton::getTypeId() const {
  return TypeTraits<UIButton>::getTypeId();
}

// -- Pointer events ----------------------------------------------------------

void UIButton::triggerPointerEnter(sf::Vector2f position) {
  m_visualState = VisualState::kHovered;
  m_visualDirty = true;
  UIWidget::triggerPointerEnter(position);
}

void UIButton::triggerPointerExit(sf::Vector2f position) {
  m_visualState = VisualState::kNormal;
  m_visualDirty = true;
  UIWidget::triggerPointerExit(position);
}

void UIButton::triggerPointerDown(sf::Vector2f position) {
  m_visualState = VisualState::kPressed;
  m_visualDirty = true;
  UIWidget::triggerPointerDown(position);
}

void UIButton::triggerPointerUp(sf::Vector2f position) {
  m_visualState = VisualState::kHovered;
  m_visualDirty = true;
  UIWidget::triggerPointerUp(position);
}

// -- Focus -------------------------------------------------------------------

void UIButton::triggerSelect() {
  m_visualDirty = true;
  UIWidget::triggerSelect();
}

void UIButton::triggerDeselect() {
  m_visualState = VisualState::kNormal;
  m_visualDirty = true;
  UIWidget::triggerDeselect();
}

void UIButton::triggerSubmit() {
  // Keyboard/gamepad activation: flash the pressed visual while the submit
  // event fires, restore (resolveColor falls through to the focused colour
  // while this button is the selection), then fire the same click event a
  // pointer press+release would — subscribers can't tell them apart.
  m_visualState = VisualState::kPressed;
  m_visualDirty = true;
  UIWidget::triggerSubmit();
  m_visualState = VisualState::kNormal;
  m_visualDirty = true;
  UIWidget::triggerPointerClick(getPosition() + getSize() * 0.5f);
}

// -- Drawing -----------------------------------------------------------------

void UIButton::syncVisual() const {
  m_shape.setSize(getSize());
  m_shape.setPosition(getPosition());
  m_shape.setFillColor(resolveColor());
  m_visualDirty = false;
}

void UIButton::onDraw(sf::RenderTarget& target, sf::RenderStates states) const {
  if (!isVisible()) { return; }

  if (m_visualDirty) { syncVisual(); }

  target.draw(m_shape, states);
}

sf::Color UIButton::resolveColor() const {
  if (!isEnabled()) {
    return m_disabledColor;
  }

  // Priority: Pressed > Hovered > Focused > Normal
  switch (m_visualState) {
  case VisualState::kPressed:  return m_pressedColor;
  case VisualState::kHovered:  return m_hoveredColor;
  case VisualState::kFocused:
  case VisualState::kDisabled:
  case VisualState::kNormal:   break;
  }

  if (isFocused()) {
    return m_focusedColor;
  }

  return m_normalColor;
}

// -- Serialization ------------------------------------------------------------

void
UIButton::onSerialize(DataStream& stream) const {
  // Version 4: shared base state (flags/rect/anchors/colour) moved into
  // UIWidget::serializeBase; v3 and older payloads no longer parse.
  constexpr uint32 kVersion = 4;
  stream << kVersion;
  serializeBase(stream);

  // Button colour overrides
  stream << m_normalColor.r   << m_normalColor.g
         << m_normalColor.b   << m_normalColor.a;
  stream << m_hoveredColor.r  << m_hoveredColor.g
         << m_hoveredColor.b  << m_hoveredColor.a;
  stream << m_pressedColor.r  << m_pressedColor.g
         << m_pressedColor.b  << m_pressedColor.a;
  stream << m_focusedColor.r  << m_focusedColor.g
         << m_focusedColor.b  << m_focusedColor.a;
  stream << m_disabledColor.r << m_disabledColor.g
         << m_disabledColor.b << m_disabledColor.a;
}

void
UIButton::onDeserialize(DataStream& stream) {
  uint32 version = 0;
  stream >> version;
  if (version != 4) {
    return;
  }
  deserializeBase(stream);

  // Button colour overrides
  stream >> m_normalColor.r   >> m_normalColor.g
         >> m_normalColor.b   >> m_normalColor.a;
  stream >> m_hoveredColor.r  >> m_hoveredColor.g
         >> m_hoveredColor.b  >> m_hoveredColor.a;
  stream >> m_pressedColor.r  >> m_pressedColor.g
         >> m_pressedColor.b  >> m_pressedColor.a;
  stream >> m_focusedColor.r  >> m_focusedColor.g
         >> m_focusedColor.b  >> m_focusedColor.a;
  stream >> m_disabledColor.r >> m_disabledColor.g
         >> m_disabledColor.b >> m_disabledColor.a;
}

} // namespace sfmx
