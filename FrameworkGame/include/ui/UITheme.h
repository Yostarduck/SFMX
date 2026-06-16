#pragma once

#include <SFML/Graphics/Color.hpp>

#include "core/platform/Prerequisites.h"

namespace sfmx {

struct UITheme {
  // Panel
  sf::Color panelFill   = {45, 45, 48};
  sf::Color panelBorder = {60, 60, 65};
  float panelBorderWidth = 1.f;
  float panelRadius     = 4.f;

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
