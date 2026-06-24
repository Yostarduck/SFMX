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
 * Each widget can have children (forming a hierarchy) but the Canvas owns the
 * root-level widgets directly.  The Canvas also carries a 2D transform
 * (sf::Transform) that is baked into the render state and used to convert
 * screen-space pointer coordinates to widget-local space during hit-testing.
 *
 * Typical usage:
 * @code
 *   auto canvas = makeUnique<Canvas>();
 *   auto btn = canvas->createWidget<UIButton>("MyButton");
 *   // ...
 *   canvas->draw(target, states);
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

  NODISCARD const String& getName() const { return m_name; }
  void setName(StringView name) { m_name = name; }

  // -- Depth / sort order ----------------------------------------------------

  /** @brief Sorting depth (higher = on top). */
  NODISCARD int getDepth() const { return m_depth; }
  void setDepth(int depth) { m_depth = depth; }

  // -- Transform -------------------------------------------------------------

  NODISCARD const sf::Transform& getTransform() const { return m_transform; }
  void setTransform(const sf::Transform& transform) { m_transform = transform; }

  // -- Widget management -----------------------------------------------------

  /**
   * @brief Create a root-level widget and return a pointer to it.
   *
   * The widget is owned by the Canvas and destroyed when the Canvas dies or
   * when destroyWidget() is called.
   */
  template<typename T, typename... Args>
  T* createWidget(Args&&... args)
  {
    static_assert(std::is_base_of_v<UIWidget, T>,
                  "T must derive from UIWidget");

    auto widget = std::make_unique<T>(std::forward<Args>(args)...);
    widget->setCanvas(this);
    T* ptr = widget.get();
    m_widgets.push_back(std::move(widget));
    return ptr;
  }

  /**
   * @brief Remove @p widget from the Canvas and destroy it recursively.
   * @return True if the widget was found and removed.
   */
  bool destroyWidget(const UIWidget* widget);

  /** @brief Remove (destroy) every widget. */
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

  Vector<std::unique_ptr<UIWidget>> m_widgets;
};

} // namespace sfmx
