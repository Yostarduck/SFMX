#pragma once

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>

#include "core/physics/Collider.h"
#include "core/platform/Prerequisites.h"
#include "utils/EventSystem.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

class Canvas;


enum class WidgetType : uint8
{
  kUnknown,
  kButton
};

/**
 * @brief Base class for every UI element placed on a Canvas or attached to a
 *        SceneNode through a component.
 *
 * Provides a local-space rectangle (m_rect), Unity-style anchor & pivot for
 * layout, an enable / visible / interactable state machine, a hierarchy of
 * parent and children, and virtual event callbacks that fire corresponding
 * Event<> members (subscribable via RAII HEvent handles).
 *
 * Hit-testing (containsPoint) can optionally use the physics collider system
 * (ICollider / intersect dispatch) instead of the raw m_rect.  Call one of the
 * setCollider*() methods or syncColliderToRect() to enable it.
 *
 * The event flow mirrors the InputAction pattern: override the virtual
 * onPointerXxx to change widget behaviour; the base implementation fires
 * Event<> subscribers so external code can connect without subclassing.
 */
class UIWidget
{
 public:
  UIWidget();
  virtual ~UIWidget();

  UIWidget(const UIWidget&) = delete;
  UIWidget& operator=(const UIWidget&) = delete;

  // -- Identity & state ------------------------------------------------------

  /** @brief The widget's display name (not required to be unique). */
  NODISCARD FORCEINLINE const String& getName() const { return m_name; }

  /** @brief Override the widget's display name. */
  FORCEINLINE void setName(StringView name) { m_name = name; }

  /** @brief Concrete shape discriminator (fast enum path; also the serialized tag) */
  NODISCARD virtual WidgetType getType() const = 0;

  /** @brief Type UUID for serialization (see TypeTraits). */
  NODISCARD virtual UUID getTypeId() const = 0;

  /** @brief True if the widget and its callbacks are processed. */
  NODISCARD FORCEINLINE bool isEnabled() const { return m_enabled; }
  FORCEINLINE void setEnabled(bool enabled) { m_enabled = enabled; }

  /** @brief True if the widget participates in rendering. */
  NODISCARD FORCEINLINE bool isVisible() const { return m_visible; }
  FORCEINLINE void setVisible(bool visible) { m_visible = visible; }

  /** @brief True if the widget can receive pointer events. */
  NODISCARD FORCEINLINE bool isInteractable() const { return m_interactable; }
  FORCEINLINE void setInteractable(bool interactable) { m_interactable = interactable; }

  /** @brief True when this widget is the current EventSystem selection. */
  NODISCARD FORCEINLINE bool isFocused() const { return m_focused; }
  FORCEINLINE void setFocused(bool focused) { m_focused = focused; }

  /** @brief Whether this widget blocks pointer events from reaching widgets drawn below it. */
  NODISCARD FORCEINLINE bool isBlockingInput() const { return m_blocksInput; }
  FORCEINLINE void setBlocksInput(bool blocks) { m_blocksInput = blocks; }

  // -- Rect ------------------------------------------------------------------

  /** @brief Local-space position (relative to parent or canvas). */
  NODISCARD FORCEINLINE sf::Vector2f getPosition() const { return m_rect.position; }
  FORCEINLINE void setPosition(sf::Vector2f position) { m_rect.position = position; }

  /** @brief Widget size in local-space units. */
  NODISCARD FORCEINLINE sf::Vector2f getSize() const { return m_rect.size; }
  FORCEINLINE void setSize(sf::Vector2f size) { m_rect.size = size; }

  /** @brief Full bounding rectangle (position + size). */
  NODISCARD FORCEINLINE const sf::FloatRect& getRect() const { return m_rect; }
  FORCEINLINE void setRect(const sf::FloatRect& rect) { m_rect = rect; }

  // -- Anchor & pivot (Unity-style layout) -----------------------------------

  /** @brief Normalized anchor minimum (0-1, fraction of parent size). */
  NODISCARD FORCEINLINE sf::Vector2f getAnchorMin() const { return m_anchorMin; }
  FORCEINLINE void setAnchorMin(sf::Vector2f min) { m_anchorMin = min; }

  /** @brief Normalized anchor maximum (0-1, fraction of parent size). */
  NODISCARD FORCEINLINE sf::Vector2f getAnchorMax() const { return m_anchorMax; }
  FORCEINLINE void setAnchorMax(sf::Vector2f max) { m_anchorMax = max; }

  /** @brief Pivot point as fraction of size (0=bottom-left, 1=top-right). */
  NODISCARD FORCEINLINE sf::Vector2f getPivot() const { return m_pivot; }
  FORCEINLINE void setPivot(sf::Vector2f pivot) { m_pivot = pivot; }

  // -- Visual ----------------------------------------------------------------

  /** @brief Tint / fill colour. */
  NODISCARD FORCEINLINE sf::Color getColor() const { return m_color; }
  FORCEINLINE void setColor(sf::Color color) { m_color = color; }

  // -- Hierarchy -------------------------------------------------------------

  /** @brief Parent widget in the UI hierarchy, or nullptr. */
  NODISCARD FORCEINLINE UIWidget* getParent() const { return m_parent; }

  /** @brief Reparent this widget under @p parent (removes from old parent). */
  void setParent(UIWidget* parent);

  /** @brief Number of direct children. */
  NODISCARD FORCEINLINE size_t getChildCount() const { return m_children.size(); }

  /** @brief Indexed access to child (nullptr if out of range). */
  NODISCARD UIWidget* getChild(size_t index) const;

  // -- Canvas ----------------------------------------------------------------

  /** @brief The Canvas that owns this widget, or nullptr. */
  NODISCARD FORCEINLINE Canvas* getCanvas() const { return m_canvas; }

  // -- Collider (optional) ---------------------------------------------------

  /**
   * @brief Replace the hit-test shape with a circle.
   * @param center  Local-space center.
   * @param radius  Circle radius.  Defaults to centering on the widget rect.
   */
  void setColliderCircle(const sf::Vector2f& center, float radius);
  FORCEINLINE void setColliderCircle(float radius) { setColliderCircle({0.f, 0.f}, radius); }

  /** @brief Replace the hit-test shape with an axis-aligned box. */
  void setColliderAABB(const sf::Vector2f& center, const sf::Vector2f& halfSize);
  FORCEINLINE void setColliderAABB(const sf::Vector2f& halfSize)
    { setColliderAABB({0.f, 0.f}, halfSize); }

  /** @brief Replace the hit-test shape with an oriented box. */
  void setColliderOBB(const sf::Vector2f& center, const sf::Vector2f& halfSize);
  FORCEINLINE void setColliderOBB(const sf::Vector2f& halfSize)
    { setColliderOBB({0.f, 0.f}, halfSize); }

  /** @brief Replace the hit-test shape with a point. */
  void setColliderPoint(const sf::Vector2f& localPos);
  FORCEINLINE void setColliderPoint() { setColliderPoint({0.f, 0.f}); }

  /** @brief Replace the hit-test shape with a line segment. */
  void setColliderLine(const sf::Vector2f& localStart,
                       const sf::Vector2f& localEnd);

  /** @brief Remove the collider; hit-testing falls back to m_rect. */
  void clearCollider();

  /**
   * @brief Convenience: set an AABB collider that matches m_rect.
   *
   * Call after changing size or position to keep the collider in sync.
   */
  void syncColliderToRect();

  /** @brief The current collider, or nullptr if none. */
  NODISCARD FORCEINLINE ICollider* getCollider() const { return m_collider.get(); }

  // -- Hit testing -----------------------------------------------------------

  /**
   * @brief True if @p point (in local space) lies inside this widget.
   *
   * If a collider has been set (via setCollider* or syncColliderToRect) the
   * physics intersect() dispatch is used; otherwise falls back to the raw
   * m_rect.contains() check.
   */
  NODISCARD virtual bool containsPoint(sf::Vector2f point) const;

  // -- Virtual event callbacks -----------------------------------------------

  /**
   * @brief Called when the pointer enters this widget's area.
   *
   * Base implementation fires the @ref onPointerEnter Event<>.
   * Override to add widget-specific behaviour (e.g. hover highlight).
   */
  virtual void onPointerEnter(sf::Vector2f position);

  /**
   * @brief Called when the pointer leaves this widget's area.
   *
   * Base implementation fires the @ref onPointerExit Event<>.
   */
  virtual void onPointerExit(sf::Vector2f position);

  /**
   * @brief Called when a pointer button is pressed over this widget.
   *
   * Base implementation fires the @ref onPointerDown Event<>.
   */
  virtual void onPointerDown(sf::Vector2f position);

  /**
   * @brief Called when a pointer button is released over this widget.
   *
   * Base implementation fires the @ref onPointerUp Event<>.
   */
  virtual void onPointerUp(sf::Vector2f position);

  /**
   * @brief Called when a click (down + up on same widget) completes.
   *
   * Base implementation fires the @ref onPointerClick Event<>.
   */
  virtual void onPointerClick(sf::Vector2f position);

  /** @brief Called when this widget becomes the EventSystem selection. */
  virtual void onSelect();

  /** @brief Called when this widget loses the EventSystem selection. */
  virtual void onDeselect();

  /** @brief Called when the user presses the submit/confirm action. */
  virtual void onSubmit();

  /** @brief Called when the user presses the cancel/back action. */
  virtual void onCancel();

  // -- Navigation links (explicit neighbor) -----------------------------------

  FORCEINLINE void setNavUp(UIWidget* widget) { m_navUp = widget; }
  FORCEINLINE void setNavDown(UIWidget* widget) { m_navDown = widget; }
  FORCEINLINE void setNavLeft(UIWidget* widget) { m_navLeft = widget; }
  FORCEINLINE void setNavRight(UIWidget* widget) { m_navRight = widget; }

  NODISCARD FORCEINLINE UIWidget* getNavUp() const { return m_navUp; }
  NODISCARD FORCEINLINE UIWidget* getNavDown() const { return m_navDown; }
  NODISCARD FORCEINLINE UIWidget* getNavLeft() const { return m_navLeft; }
  NODISCARD FORCEINLINE UIWidget* getNavRight() const { return m_navRight; }

  // -- Public Event connect methods (InputAction-style RAII handles) ----------

  /** @brief Subscribe to pointer-enter. Returns an RAII unsubscribe handle. */
  NODISCARD FORCEINLINE HEvent onPointerEnter(Function<void(sf::Vector2f)> cb) const
    { return m_onPointerEnterEvent.connect(std::move(cb)); }

  /** @brief Subscribe to pointer-exit. Returns an RAII unsubscribe handle. */
  NODISCARD FORCEINLINE HEvent onPointerExit(Function<void(sf::Vector2f)> cb) const
    { return m_onPointerExitEvent.connect(std::move(cb)); }

  /** @brief Subscribe to pointer-down. Returns an RAII unsubscribe handle. */
  NODISCARD FORCEINLINE HEvent onPointerDown(Function<void(sf::Vector2f)> cb) const
    { return m_onPointerDownEvent.connect(std::move(cb)); }

  /** @brief Subscribe to pointer-up. Returns an RAII unsubscribe handle. */
  NODISCARD FORCEINLINE HEvent onPointerUp(Function<void(sf::Vector2f)> cb) const
    { return m_onPointerUpEvent.connect(std::move(cb)); }

  /** @brief Subscribe to click. Returns an RAII unsubscribe handle. */
  NODISCARD FORCEINLINE HEvent onPointerClick(Function<void(sf::Vector2f)> cb) const
    { return m_onPointerClickEvent.connect(std::move(cb)); }

  /** @brief Subscribe to selection. Returns an RAII unsubscribe handle. */
  NODISCARD FORCEINLINE HEvent onSelect(Function<void()> cb) const
    { return m_onSelectEvent.connect(std::move(cb)); }

  /** @brief Subscribe to deselection. Returns an RAII unsubscribe handle. */
  NODISCARD FORCEINLINE HEvent onDeselect(Function<void()> cb) const
    { return m_onDeselectEvent.connect(std::move(cb)); }

  /** @brief Subscribe to submit. Returns an RAII unsubscribe handle. */
  NODISCARD FORCEINLINE HEvent onSubmit(Function<void()> cb) const
    { return m_onSubmitEvent.connect(std::move(cb)); }

  /** @brief Subscribe to cancel. Returns an RAII unsubscribe handle. */
  NODISCARD FORCEINLINE HEvent onCancel(Function<void()> cb) const
    { return m_onCancelEvent.connect(std::move(cb)); }

  // -- Drawing ---------------------------------------------------------------

  /**
   * @brief Draw this widget onto @p target.
   *
   * @param target The surface to draw onto.
   * @param states Render states carrying the accumulated transform of the
   *               parent canvas / component chain.
   */
  virtual void onDraw(sf::RenderTarget& target, sf::RenderStates states) const;

 protected:
  friend class Canvas;
  friend class UIEventSystem;

  FORCEINLINE void setCanvas(Canvas* canvas) { m_canvas = canvas; }

  // Event members (private — fired by the base virtual callback implementations)
  Event<void(sf::Vector2f)> mutable m_onPointerEnterEvent;
  Event<void(sf::Vector2f)> mutable m_onPointerExitEvent;
  Event<void(sf::Vector2f)> mutable m_onPointerDownEvent;
  Event<void(sf::Vector2f)> mutable m_onPointerUpEvent;
  Event<void(sf::Vector2f)> mutable m_onPointerClickEvent;
  Event<void()> mutable m_onSelectEvent;
  Event<void()> mutable m_onDeselectEvent;
  Event<void()> mutable m_onSubmitEvent;
  Event<void()> mutable m_onCancelEvent;

  String m_name;
  bool m_enabled = true;
  bool m_visible = true;
  bool m_interactable = true;
  bool m_focused = false;
  bool m_blocksInput = true;

  sf::FloatRect m_rect;        // position + size (local space)
  sf::Vector2f m_anchorMin;    // Unity-style normalized 0-1
  sf::Vector2f m_anchorMax;
  sf::Vector2f m_pivot;        // (0,0)=bottom-left, (1,1)=top-right, (0.5,0.5)=center

  sf::Color m_color = sf::Color::White;

  UniquePtr<ICollider> m_collider;

  UIWidget* m_parent = nullptr;
  Vector<UIWidget*> m_children;
  Canvas* m_canvas = nullptr;

  // Navigation links (explicit neighbours, raw pointers — owned by Canvas).
  UIWidget* m_navUp = nullptr;
  UIWidget* m_navDown = nullptr;
  UIWidget* m_navLeft = nullptr;
  UIWidget* m_navRight = nullptr;
};

template<typename Derived, WidgetType Type>
class UIWidgetT : public UIWidget
{
 public:
  UIWidgetT() : UIWidget() {}
  NODISCARD FORCEINLINE virtual WidgetType getType() const override { return Type; }
  NODISCARD FORCEINLINE virtual UUID getTypeId() const override { return TypeTraits<Derived>::getTypeId(); }
};


} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UIWidget)
