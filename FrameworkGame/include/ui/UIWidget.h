/************************************************************************/
/**
 * @file UIWidget.h
 * @author Swampertor
 * @date 2026/06/10
 * @brief  Base class for every UI element owned by the UIManager.
 */
/************************************************************************/
#pragma once

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>

#include "core/platform/Prerequisites.h"
#include "utils/EventSystem.h"
#include "utils/TypeTraits.h"
#include "ui/UISlot.h"

namespace sfmx
{

class UIManager;
class DataStream;


enum class WidgetType : uint8
{
  kUnknown,
  kButton,
  kLabel,
  kImage,
  kCheckbox,
  kTextBox,
  kSlider,
  kVerticalBox,
  kHorizontalBox,
  kScrollView
};

/**
 * @brief Base class for every UI element in the UI document tree.
 *
 * Provides a local-space rectangle (m_rect), Unity-style anchor & pivot for
 * layout, an enable / visible / interactable state machine, a hierarchy of
 * parent and children, and virtual event callbacks that fire corresponding
 * Event<> members (subscribable via RAII HEvent handles).
 *
 * Hit-testing (containsPoint) is a plain m_rect check.
 *
 * Ownership: every widget is either a ROOT registered with the UIManager
 * (m_manager set on the widget itself) or a CHILD of a container widget
 * (m_parent set). A parent destroys its children; the UIManager destroys its
 * roots. Destruction goes through the widget's memory pool (erased
 * MemoryPoolHandler::deallocate), never `delete`.
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

  /** @brief Concrete shape discriminator (fast enum path; also the serialized tag) */
  NODISCARD virtual WidgetType
  getType() const = 0;

  /** @brief Type UUID for serialization (see TypeTraits). */
  NODISCARD virtual UUID
  getTypeId() const = 0;

  /** @brief Unique-ish lookup name (what `UI:get(name)` searches for). */
  NODISCARD FORCEINLINE const String&
  getName() const { return m_name; }
  FORCEINLINE void
  setName(StringView name) { m_name = name; }

  /** @brief True if the widget and its callbacks are processed. */
  NODISCARD FORCEINLINE bool
  isEnabled() const { return m_enabled; }
  FORCEINLINE void
  setEnabled(bool enabled) { m_enabled = enabled; }

  /** @brief True if the widget participates in rendering. */
  NODISCARD FORCEINLINE bool
  isVisible() const { return m_visible; }
  FORCEINLINE void
  setVisible(bool visible) { m_visible = visible; }

  /** @brief True if the widget can receive pointer events. */
  NODISCARD FORCEINLINE bool
  isInteractable() const { return m_interactable; }
  FORCEINLINE void
  setInteractable(bool interactable) { m_interactable = interactable; }

  /** @brief True when this widget is the current UIManager selection. */
  NODISCARD FORCEINLINE bool
  isFocused() const { return m_focused; }
  FORCEINLINE void
  setFocused(bool focused) { m_focused = focused; }

  /** @brief Whether this widget blocks pointer events from reaching widgets drawn below it. */
  NODISCARD FORCEINLINE bool
  isBlockingInput() const { return m_blocksInput; }
  FORCEINLINE void
  setBlocksInput(bool blocks) { m_blocksInput = blocks; }

  // -- Rect ------------------------------------------------------------------

  /** @brief Local-space position (relative to parent, or window pixels at root level). */
  NODISCARD FORCEINLINE sf::Vector2f
  getPosition() const { return m_rect.position; }
  // Also rewrites the slot offset: raw placement is the offset datum for
  // default anchors (authoring order: set anchors + the final offset AFTER
  // positioning — see ui/UISlot.h).
  //
  // The three setters are VIRTUAL: widgets that cache geometry
  // (UIButton/UICheckbox/UISlider rebuild only when marked dirty) must
  // override them — UIManager::relayout moves roots through a UIWidget*,
  // and a name-hiding "override" would leave stale pixels drawn at the old
  // position while hit-testing follows the moved rect.
  FORCEINLINE virtual void
  setPosition(sf::Vector2f position) {
    m_rect.position = position;
    m_slot.offset = position;
  }

  /** @brief Widget size in local-space units. */
  NODISCARD FORCEINLINE sf::Vector2f
  getSize() const { return m_rect.size; }
  FORCEINLINE virtual void
  setSize(sf::Vector2f size) { m_rect.size = size; }

  /** @brief Full bounding rectangle (position + size). */
  NODISCARD FORCEINLINE const sf::FloatRect&
  getRect() const { return m_rect; }
  FORCEINLINE virtual void
  setRect(const sf::FloatRect& rect) {
    m_rect = rect;
    m_slot.offset = rect.position;
  }

  // -- Slot: anchors, pivot, offset (Unity-style layout) ----------------------
  // APPLIED only at root level: the implicit viewport-spanning root panel
  // calls UIManager::relayout (sf::Event::Resized hook, and addRoot while the
  // viewport is known), which recomputes each root's position from its slot.
  // Defaults (anchors {0,0}/{0,0}, pivot {0,0}) make offset == absolute
  // top-left position, so manually positioned setups keep working unchanged.
  // Inside boxes / ScrollView the slot rides along but is IGNORED — their
  // auto-layout (padding/spacing) owns child positions.
  // Authoring order: setPosition/setRect rewrite the offset, so set anchors
  // and the anchor-relative offset AFTER positioning (see ui/UISlot.h).

  /** @brief Full layout slot (anchors, pivot, offset) of this widget. */
  NODISCARD FORCEINLINE const UISlot&
  getSlot() const { return m_slot; }
  FORCEINLINE void
  setSlot(const UISlot& slot) { m_slot = slot; }

  /** @brief Normalized anchor minimum (0-1, fraction of viewport at root level). */
  NODISCARD FORCEINLINE sf::Vector2f
  getAnchorMin() const { return m_slot.anchorMin; }
  FORCEINLINE void
  setAnchorMin(sf::Vector2f min) { m_slot.anchorMin = min; }

  /** @brief Normalized anchor maximum (0-1, fraction of viewport at root level). */
  NODISCARD FORCEINLINE sf::Vector2f
  getAnchorMax() const { return m_slot.anchorMax; }
  FORCEINLINE void
  setAnchorMax(sf::Vector2f max) { m_slot.anchorMax = max; }

  /** @brief Pivot as fraction of size (0,0 = top-left ... 1,1 = bottom-right). */
  NODISCARD FORCEINLINE sf::Vector2f
  getPivot() const { return m_slot.pivot; }
  FORCEINLINE void
  setPivot(sf::Vector2f pivot) { m_slot.pivot = pivot; }

  /** @brief Pivot position relative to the anchor reference (rewritten by setPosition/setRect). */
  NODISCARD FORCEINLINE sf::Vector2f
  getOffset() const { return m_slot.offset; }
  FORCEINLINE void
  setOffset(sf::Vector2f offset) { m_slot.offset = offset; }

  // -- Visual ----------------------------------------------------------------

  /** @brief Tint / fill colour. */
  NODISCARD FORCEINLINE sf::Color
  getColor() const { return m_color; }
  FORCEINLINE void
  setColor(sf::Color color) { m_color = color; }

  // -- Manager ---------------------------------------------------------------

  /** @brief The UIManager that owns this widget's root, or nullptr.
   *         Walks the parent chain so hierarchy children find the manager. */
  NODISCARD FORCEINLINE UIManager*
  getManager() const {
    if (m_manager != nullptr) return m_manager;
    return m_parent != nullptr ? m_parent->getManager() : nullptr;
  }

  // -- Hit testing -----------------------------------------------------------

  /**
   * @brief True if @p point (in local space) lies inside this widget.
   *
   * Rect-only check (`m_rect.contains`).
   */
  NODISCARD virtual bool
  containsPoint(sf::Vector2f point) const;

  // -- Virtual event callbacks -----------------------------------------------

  /**
   * @brief Called when the pointer enters this widget's area.
   *
   * Base implementation fires the @ref onPointerEnter Event<>.
   * Override to add widget-specific behaviour (e.g. hover highlight).
   */
  virtual void
  triggerPointerEnter(sf::Vector2f position);

  /**
   * @brief Called when the pointer leaves this widget's area.
   *
   * Base implementation fires the @ref onPointerExit Event<>.
   */
  virtual void
  triggerPointerExit(sf::Vector2f position);

  /**
   * @brief Called when a pointer button is pressed over this widget.
   *
   * Base implementation fires the @ref onPointerDown Event<>.
   */
  virtual void
  triggerPointerDown(sf::Vector2f position);

  /**
   * @brief Called when a pointer button is released over this widget.
   *
   * Base implementation fires the @ref onPointerUp Event<>.
   */
  virtual void
  triggerPointerUp(sf::Vector2f position);

  /**
   * @brief Called when a click (down + up on same widget) completes.
   *
   * Base implementation fires the @ref onPointerClick Event<>.
   */
  virtual void
  triggerPointerClick(sf::Vector2f position);

  /** @brief Called when the scroll wheel is used over this widget or its children. */
  virtual void
  triggerScroll(float delta);

  /** @brief Called when this widget becomes the UIManager selection. */
  virtual void
  triggerSelect();

  /** @brief Called when this widget loses the UIManager selection. */
  virtual void
  triggerDeselect();

  /** @brief Called when the user presses the submit/confirm action. */
  virtual void
  triggerSubmit();

  /** @brief Called when the user presses the cancel/back action. */
  virtual void
  triggerCancel();

  /** @brief Whether this widget is a text editor (skips navigation while focused). */
  NODISCARD virtual bool
  isTextEditor() const { return false; }

  // -- Hierarchy (parent / children) -----------------------------------------

  /** @brief The containing widget, or nullptr if this is a root widget. */
  NODISCARD FORCEINLINE UIWidget* getUIparent() const { return m_parent; }

  /** @brief Direct children managed by this container widget. */
  NODISCARD FORCEINLINE const Vector<UIWidget*>& getChildren() const { return m_children; }

  /**
   * @brief Add a child managed by this container widget.
   *
   * The child must NOT be a root registered with the UIManager.
   * A parent owns its children: destroying this widget destroys the child
   * through its memory pool.
   */
  void addChild(UIWidget* child);

  /** @brief Remove a child without destroying it. */
  void removeChild(UIWidget* child);

  /**
   * @brief The local-space transform applied to children during drawing and
   *        hit-testing.  Override in containers (e.g. ScrollView) to offset
   *        children by the content-scroll amount.
   */
  NODISCARD virtual sf::Transform getChildTransform() const;

  // -- Hierarchy traversal (overridden by containers) -------------------------

  /**
   * @brief Draw this widget and (recursively) its children.
   *
   * Default calls @ref onDraw.  Containers override to draw children with
   * @ref getChildTransform baked into the render states.
   */
  virtual void drawHierarchy(sf::RenderTarget& target, sf::RenderStates states) const;

  /**
   * @brief Recursive hit-test through this widget and its children.
   *
   * Takes a point in the frame this widget's rect lives in (root space for
   * roots, the parent's child frame for children). Returns the topmost widget
   * under the point — recursion maps the point into each child's frame with
   * the INVERSE of @ref getChildTransform, the exact reverse of the
   * composition @ref drawHierarchy applies forward, so hit-testing mirrors
   * drawing exactly. Invisible widgets return nullptr;
   * disabled / non-interactable widgets return this (they block widgets drawn
   * underneath but never receive pointer events — the UIManager gates
   * dispatch). Blocking vs. fall-through (@ref isBlockingInput) is decided by
   * the caller (UIManager::hitTest).
   */
  NODISCARD virtual UIWidget* hitTestInHierarchy(sf::Vector2f point) const;

  /**
   * @brief Convert a root-space (window pixel) point to this widget's local space.
   *
   * Default walks the parent chain subtracting each position.
   * Containers override to account for their own position + scroll offset.
   */
  NODISCARD virtual sf::Vector2f toLocalSpace(sf::Vector2f canvasPoint) const;

  // -- Navigation links (explicit neighbor) -----------------------------------

  FORCEINLINE void
  setNavUp(UIWidget* widget) { m_navUp = widget; }
  FORCEINLINE void
  setNavDown(UIWidget* widget) { m_navDown = widget; }
  FORCEINLINE void
  setNavLeft(UIWidget* widget) { m_navLeft = widget; }
  FORCEINLINE void
  setNavRight(UIWidget* widget) { m_navRight = widget; }

  NODISCARD FORCEINLINE UIWidget*
  getNavUp() const { return m_navUp; }
  NODISCARD FORCEINLINE UIWidget*
  getNavDown() const { return m_navDown; }
  NODISCARD FORCEINLINE UIWidget*
  getNavLeft() const { return m_navLeft; }
  NODISCARD FORCEINLINE UIWidget*
  getNavRight() const { return m_navRight; }

  // -- Public Event connect methods (InputAction-style RAII handles) ----------

  /** @brief Subscribe to pointer-enter. Returns an RAII unsubscribe handle. */
  NODISCARD FORCEINLINE HEvent
  onPointerEnter(Function<void(sf::Vector2f)> cb) const
  { return m_onPointerEnterEvent.connect(std::move(cb)); }

  /** @brief Subscribe to pointer-exit. Returns an RAII unsubscribe handle. */
  NODISCARD FORCEINLINE HEvent
  onPointerExit(Function<void(sf::Vector2f)> cb) const
  { return m_onPointerExitEvent.connect(std::move(cb)); }

  /** @brief Subscribe to pointer-down. Returns an RAII unsubscribe handle. */
  NODISCARD FORCEINLINE HEvent
  onPointerDown(Function<void(sf::Vector2f)> cb) const
  { return m_onPointerDownEvent.connect(std::move(cb)); }

  /** @brief Subscribe to pointer-up. Returns an RAII unsubscribe handle. */
  NODISCARD FORCEINLINE HEvent
  onPointerUp(Function<void(sf::Vector2f)> cb) const
  { return m_onPointerUpEvent.connect(std::move(cb)); }

  /** @brief Subscribe to click. Returns an RAII unsubscribe handle. */
  NODISCARD FORCEINLINE HEvent
  onPointerClick(Function<void(sf::Vector2f)> cb) const
  { return m_onPointerClickEvent.connect(std::move(cb)); }

  /** @brief Subscribe to selection. Returns an RAII unsubscribe handle. */
  NODISCARD FORCEINLINE HEvent
  onSelect(Function<void()> cb) const
  { return m_onSelectEvent.connect(std::move(cb)); }

  /** @brief Subscribe to deselection. Returns an RAII unsubscribe handle. */
  NODISCARD FORCEINLINE HEvent
  onDeselect(Function<void()> cb) const
  { return m_onDeselectEvent.connect(std::move(cb)); }

  /** @brief Subscribe to submit. Returns an RAII unsubscribe handle. */
  NODISCARD FORCEINLINE HEvent
  onSubmit(Function<void()> cb) const
  { return m_onSubmitEvent.connect(std::move(cb)); }

  /** @brief Subscribe to cancel. Returns an RAII unsubscribe handle. */
  NODISCARD FORCEINLINE HEvent
  onCancel(Function<void()> cb) const
  { return m_onCancelEvent.connect(std::move(cb)); }

  // -- Lifecycle --------------------------------------------------------------

  /**
   * @brief Per-frame update hook, driven recursively by UIManager::update.
   * @param deltaTime Seconds elapsed since the previous frame.
   */
  virtual void
  onUpdate(float deltaTime);

  /**
   * @brief Draw this widget onto @p target.
   *
   * @param target The surface to draw onto.
   * @param states Render states carrying the accumulated transform of the
   *               parent widget chain.
   */
  virtual void
  onDraw(sf::RenderTarget& target, sf::RenderStates states) const;

  /** @brief Write this widget's persistent state (UI document, Phase 5). Default: nothing. */
  virtual void
  onSerialize(DataStream& stream) const { SFMX_PARAMETER_UNUSED(stream); }

  /** @brief Read state written by @ref onSerialize. Default: nothing. */
  virtual void
  onDeserialize(DataStream& stream) { SFMX_PARAMETER_UNUSED(stream); }

  /**
   * @brief Write the shared base state every widget round-trips: format
   *        version, name, enabled/visible/interactable/focused/blocksInput
   *        flags, rect, slot (anchors/pivot/offset) and colour.
   *
   * Call right AFTER your own version byte in @ref onSerialize — it keeps
   * every payload uniform and fixes widgets that used to drop their rect
   * (UILabel/UIImage) or anchors (the boxes/scroll view).
   */
  void
  serializeBase(DataStream& stream) const;

  /** @brief Counterpart of @ref serializeBase — call right after your version check. */
  void
  deserializeBase(DataStream& stream);

 protected:
  friend class UIManager;

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
  UISlot m_slot;               // anchors + pivot + offset (see ui/UISlot.h)

  sf::Color m_color = sf::Color::White;

  // Root registration back-pointer. Set ONLY on root widgets; children reach
  // the manager through getManager()'s parent-chain walk.
  UIManager* m_manager = nullptr;

  // Hierarchy (parent / children — parent owns children).
  UIWidget* m_parent = nullptr;
  Vector<UIWidget*> m_children;

  // Navigation links (explicit neighbours, raw pointers).
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
