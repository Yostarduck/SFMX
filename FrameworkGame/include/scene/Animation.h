#pragma once

#include "core/platform/Prerequisites.h"

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Transform.hpp>
#include <SFML/Graphics/Color.hpp>

namespace sfmx
{

struct Frame
{
  Frame() = default;
  
  Frame(sf::Texture& t,  
        const sf::Color& c = sf::Color::White)
    : color(c) {
        texture = &t;
        sf::Vector2i size(texture->getSize().x, texture->getSize().y);
        framing = {{0,0}, size};
      }
  
  
  Frame(sf::Texture& t, 
        const sf::IntRect& f, 
        const sf::Color& c = sf::Color::White)
    : framing(f),
      color(c) {
        texture = &t;
      }
  
  void flipX(bool flip) { flippedX = flip; }
  void flipY(bool flip) { flippedY = flip; }
  void toggleFlipX()    { flippedX = !flippedX; }
  void toggleFlipY()    { flippedY = !flippedY; }

  sf::Texture*  texture       = nullptr;
  sf::IntRect   framing       = {{0,0},{0,0}};
  sf::Vector2f  scale         = {1, 1};
  sf::Vector2f  position      = {0, 0};
  sf::Angle     rotation      = sf::degrees(0);
  sf::Color     color         = sf::Color::White;
  bool          flippedX      = false;
  bool          flippedY      = false;
};

class Animation
{
public:

  void setFrames(const Vector<Frame>& frames);

  Vector<Frame> m_frames;
  bool          m_loops;
  float         m_speedMultiplier;
  float         m_duration;

};

}