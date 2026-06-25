#pragma once

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Transform.hpp>

#include "core/platform/Prerequisites.h"
#include "ui/UIWidget.h"

namespace sfmx
{

class UIEventSystem;

/**
 * @brief Holds an ordered list of UIWidgets, sorted by depth for rendering
 *        and hit-testing.
 *
 * Each widget can have children (forming a hierarchy) but the Canvas holds
 * references to root-level widgets directly.  The Canvas also carries a 2D
 * transform (sf::Transform) that is baked into the render state and used to
 * convert screen-space pointer coordinates to widget-local space during
 * hit-testing.
 *
 * Widgets are NOT owned by the Canvas — they are pool-allocated through the
 * scene component system or stack-allocated.  Call addWidget() to register
 * a widget, and removeWidget() to unregister without destroying it.  Widgets
 * auto-unregister in their destructor.
 *
 * Typical usage:
 * @code
 *   Canvas canvas("HUD");
 *   auto* btn = node->addComponent<UIButton>(node, "MyButton", {200.f, 50.f});
 *   canvas.addWidget(btn);
 *   // ...
 *   canvas.draw(target, states);
 * @endcode
 *
 * Depth ordering: widgets with a higher (larger) depth value are drawn on top
 * and are hit-tested first.
 */
class Canvas
{
 public:
  explicit Canvas(StringView name = "Canvas");
  ~Canvas();

  Canvas(const Canvas&) = delete;
  Canvas& operator=(const Canvas&) = delete;

  // -- Identity --------------------------------------------------------------

  NODISCARD FORCEINLINE const String& getName() const { return m_name; }
  FORCEINLINE void setName(StringView name) { m_name = name; }

  // -- Depth / sort order ----------------------------------------------------

  /** @brief Sorting depth (higher = on top). */
  NODISCARD FORCEINLINE int getDepth() const { return m_depth; }
  FORCEINLINE void setDepth(int depth) { m_depth = depth; }

  // -- Transform -------------------------------------------------------------

  NODISCARD FORCEINLINE const sf::Transform& getTransform() const { return m_transform; }
  FORCEINLINE void setTransform(const sf::Transform& transform) { m_transform = transform; }

  // -- Widget management -----------------------------------------------------

  /**
   * @brief Register a widget with this Canvas.
   *
   * Sets the widget's canvas pointer and adds it to the draw/hit-test list.
   * The widget is NOT owned by the Canvas — it must be destroyed separately
   * (e.g. via its owning SceneNode).  Widgets auto-unregister in ~UIWidget().
   */
  void addWidget(UIWidget* widget);

  /**
   * @brief Remove @p widget from this Canvas without destroying it.
   * @return True if the widget was found and removed.
   */
  bool removeWidget(UIWidget* widget);

  /** @brief Remove all widget references (no destruction). */
  void clear();

  // -- Hit testing -----------------------------------------------------------

  /**
   * @brief Find the topmost interactive widget containing @p localPoint.
   *
   * @param localPoint Point in canvas-local coordinates.
   * @return The widget, or nullptr if nothing was hit.
   */
  NODISCARD UIWidget* hitTest(sf::Vector2f localPoint) const;

  // -- Drawing ---------------------------------------------------------------

  /**
   * @brief Draw all visible widgets, sorted by depth.
   *
   * NOTE: The canvas transform is composed into @p states before drawing.
   * The caller is responsible for setting up the RenderTarget view.
   */
  void draw(sf::RenderTarget& target, sf::RenderStates states) const;

 private:
  friend class UIEventSystem;

  String m_name;
  int m_depth = 0;
  sf::Transform m_transform;

  Vector<UIWidget*> m_widgets;
};

} // namespace sfmx
