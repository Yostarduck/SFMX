#pragma once
#include "core/platform/Prerequisites.h"
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>

namespace sfmx
{
struct Frame
{
  Frame() = default;

  Frame(SPtr<sf::Texture> t,
        const sf::Color& c = sf::Color::White)
    : texture(t), color(c) {
        sf::Vector2i size(texture->getSize().x, texture->getSize().y);
        framing = {{0,0}, size};
      }

  Frame(SPtr<sf::Texture> t,
        const sf::IntRect& f,
        const sf::Color& c = sf::Color::White)
    : texture(t), framing(f), color(c) {}

  SPtr<sf::Texture> texture       = nullptr;
  sf::IntRect       framing       = {{0,0},{0,0}};
  sf::Vector2f      scale         = {1, 1};
  sf::Vector2f      position      = {0, 0};
  sf::Angle         rotation      = sf::degrees(0);
  sf::Color         color         = sf::Color::White;
  bool              flippedX      = false;
  bool              flippedY      = false;
};

}
