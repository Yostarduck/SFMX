#include <doctest/doctest.h>

#include "resource/SpriteAtlas.h"

#include <SFML/Graphics/Image.hpp>

using namespace sfmx;

static auto uv = [](unsigned x, unsigned y) { return sf::Vector2u{x, y}; };
static auto sz = [](unsigned w, unsigned h) { return sf::Vector2u{w, h}; };

TEST_CASE("SpriteAtlas empty transparent image") {
  sf::Image img(sz(16, 16), sf::Color::Transparent);
  CHECK(Atlas::detectSpriteRects(img).empty());
}

TEST_CASE("SpriteAtlas single opaque pixel") {
  sf::Image img(sz(16, 16), sf::Color::Transparent);
  img.setPixel(uv(5, 7), sf::Color::White);

  const auto rects = Atlas::detectSpriteRects(img);
  REQUIRE(rects.size() == 1);
  CHECK(rects[0].position.x == 5);
  CHECK(rects[0].position.y == 7);
  CHECK(rects[0].size.x == 1);
  CHECK(rects[0].size.y == 1);
}

TEST_CASE("SpriteAtlas two disjoint squares") {
  sf::Image img(sz(32, 32), sf::Color::Transparent);

  for (int y = 2; y < 8; ++y)
    for (int x = 3; x < 9; ++x)
      img.setPixel(uv(x, y), sf::Color::White);

  for (int y = 20; y < 26; ++y)
    for (int x = 22; x < 28; ++x)
      img.setPixel(uv(x, y), sf::Color::White);

  const auto rects = Atlas::detectSpriteRects(img);
  REQUIRE(rects.size() == 2);
  CHECK(rects[0].position.x == 3);
  CHECK(rects[0].position.y == 2);
  CHECK(rects[0].size.x == 6);
  CHECK(rects[0].size.y == 6);

  CHECK(rects[1].position.x == 22);
  CHECK(rects[1].position.y == 20);
  CHECK(rects[1].size.x == 6);
  CHECK(rects[1].size.y == 6);
}

TEST_CASE("SpriteAtlas L-shape merges into one bounding rect") {
  sf::Image img(sz(16, 16), sf::Color::Transparent);

  for (int x = 2; x < 10; ++x)
    img.setPixel(uv(x, 2), sf::Color::White);
  for (int y = 2; y < 10; ++y)
    img.setPixel(uv(2, y), sf::Color::White);

  const auto rects = Atlas::detectSpriteRects(img);
  REQUIRE(rects.size() == 1);
  CHECK(rects[0].position.x == 2);
  CHECK(rects[0].position.y == 2);
  CHECK(rects[0].size.x == 8);
  CHECK(rects[0].size.y == 8);
}

TEST_CASE("SpriteAtlas diagonal touch stays separate") {
  sf::Image img(sz(16, 16), sf::Color::Transparent);

  img.setPixel(uv(4, 4), sf::Color::White);
  img.setPixel(uv(5, 5), sf::Color::White);

  CHECK(Atlas::detectSpriteRects(img).size() == 2);
}

TEST_CASE("SpriteAtlas padding expands rects") {
  sf::Image img(sz(16, 16), sf::Color::Transparent);
  img.setPixel(uv(7, 7), sf::Color::White);

  const auto rects = Atlas::detectSpriteRects(img, 1, 2);
  REQUIRE(rects.size() == 1);
  CHECK(rects[0].position.x == 5);
  CHECK(rects[0].position.y == 5);
  CHECK(rects[0].size.x == 5);
  CHECK(rects[0].size.y == 5);
}

TEST_CASE("SpriteAtlas padding clamped to image bounds") {
  sf::Image img(sz(8, 8), sf::Color::Transparent);
  img.setPixel(uv(0, 0), sf::Color::White);

  const auto rects = Atlas::detectSpriteRects(img, 1, 10);
  REQUIRE(rects.size() == 1);
  CHECK(rects[0].position.x == 0);
  CHECK(rects[0].position.y == 0);
  CHECK(rects[0].size.x == 8);
  CHECK(rects[0].size.y == 8);
}

TEST_CASE("SpriteAtlas alpha threshold filtering") {
  sf::Image img(sz(8, 8), sf::Color::Transparent);

  img.setPixel(uv(2, 2), sf::Color(255, 255, 255, 3));
  img.setPixel(uv(5, 5), sf::Color::White);

  const auto rectsLow = Atlas::detectSpriteRects(img, 1);
  CHECK(rectsLow.size() == 2);

  const auto rectsHigh = Atlas::detectSpriteRects(img, 5);
  REQUIRE(rectsHigh.size() == 1);
  CHECK(rectsHigh[0].position.x == 5);
  CHECK(rectsHigh[0].position.y == 5);
}

TEST_CASE("SpriteAtlas results sorted top-to-bottom") {
  sf::Image img(sz(16, 16), sf::Color::Transparent);

  img.setPixel(uv(4, 10), sf::Color::White);
  img.setPixel(uv(4, 2), sf::Color::White);

  const auto rects = Atlas::detectSpriteRects(img);
  REQUIRE(rects.size() == 2);
  CHECK(rects[0].position.y == 2);
  CHECK(rects[1].position.y == 10);
}

TEST_CASE("SpriteAtlas zero-size image") {
  sf::Image img;
  CHECK(Atlas::detectSpriteRects(img).empty());
}
