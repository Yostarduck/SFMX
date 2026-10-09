/************************************************************************/
/**
 * @file UISlot.h
 * @author Swampertor
 * @date 2026/09/23
 * @brief  Per-child anchor/pivot/offset layout data (Unity-style).
 */
/************************************************************************/
#pragma once

#include <SFML/System/Vector2.hpp>

#include "core/platform/Prerequisites.h"

namespace sfmx
{

/**
 * @brief Layout data a container uses to place one of its widgets.
 *
 * - anchorMin / anchorMax: normalized (0-1) reference rect — at ROOT level
 *   that is the viewport; both equal = point anchor (corner / edge / center).
 * - pivot: which point of the widget rect the layout positions (0,0 =
 *   top-left ... 1,1 = bottom-right) and, when the anchors differ, where
 *   between them the reference point lies.
 * - offset: position of the pivot relative to the anchor reference point.
 *
 * Where slots are APPLIED:
 *  - Root widgets: by the implicit viewport-spanning root panel —
 *    UIManager::relayout (from the sf::Event::Resized hook, and on addRoot
 *    while the viewport is known). Default anchors {0,0}/{0,0} + pivot
 *    {0,0} reduce to "offset = absolute top-left position", so manually
 *    positioned setups keep working unchanged.
 *  - Inside UIHorizontalBox / UIVerticalBox / UIScrollView the slot rides
 *    along on the child but the anchors are IGNORED — auto-layout
 *    (padding/spacing) owns child positions there.
 *
 * Authoring order: setPosition()/setRect() rewrite `offset` (it mirrors the
 * last raw placement), so set the anchors and the final anchor-relative
 * offset AFTER positioning the widget.
 */
struct UISlot
{
  sf::Vector2f anchorMin{0.f, 0.f};  ///< Normalized (0-1) viewport fraction
  sf::Vector2f anchorMax{0.f, 0.f};  ///< Equal to anchorMin = point anchor
  sf::Vector2f pivot{0.f, 0.f};      ///< Fraction of size: 0,0 = top-left
  sf::Vector2f offset{0.f, 0.f};     ///< Pivot relative to the anchor reference

  /**
   * @brief Top-left position for a widget of @p size inside @p viewport.
   *
   * position = ref(viewport) + offset − pivot ⊙ size, with
   * ref = lerp(anchorMin⊙viewport, anchorMax⊙viewport, pivot).
   * The size is never stretched — differing anchors only move the reference.
   */
  NODISCARD sf::Vector2f
  computePosition(const sf::Vector2f& viewport,
                  const sf::Vector2f& size) const {
    const sf::Vector2f refMin{anchorMin.x * viewport.x,
                              anchorMin.y * viewport.y};
    const sf::Vector2f refMax{anchorMax.x * viewport.x,
                              anchorMax.y * viewport.y};
    const sf::Vector2f ref{
      refMin.x + (refMax.x - refMin.x) * pivot.x,
      refMin.y + (refMax.y - refMin.y) * pivot.y};
    return {ref.x + offset.x - pivot.x * size.x,
            ref.y + offset.y - pivot.y * size.y};
  }
};

} // namespace sfmx
