/************************************************************************/
/**
 * @file UITheme.h
 * @author Swampertor
 * @date 2026/06/16
 * @brief  Default colour and style palette for UI widgets.
 */
/************************************************************************/
#pragma once

#include <SFML/Graphics/Color.hpp>

#include "core/platform/Prerequisites.h"

namespace sfmx
{

/**
 * @brief Container of default colours used by widgets when they are
 *        initialised from a UICanvasComponent's theme.
 */
struct UITheme {
  // Panel
  sf::Color panelFill        = {45, 45, 48};
  sf::Color panelBorder      = {60, 60, 65};
  float     panelBorderWidth = 1.f;
  float     panelRadius      = 4.f;

  // Button
  sf::Color buttonFill      = {60, 60, 65};
  sf::Color buttonHover     = {75, 75, 80};
  sf::Color buttonPress     = {90, 90, 95};
  sf::Color buttonTextColor = {220, 220, 220};

  // Label
  sf::Color labelTextColor = {220, 220, 220};
  unsigned  labelFontSize  = 14;
};

} // namespace sfmx
