// SpriteAtlas test group: auto-detect sprite rects from non-transparent regions.

#include "SpriteAtlasTest.h"

#include <iostream>

#include "resource/SpriteAtlas.h"
#include "TestRunner.h"

#include <SFML/Graphics/Image.hpp>

using namespace sfmx;

// SFML 3: getPixel/setPixel take sf::Vector2u instead of (x, y);
//          Image constructor takes sf::Vector2u instead of two ints.
static auto uv = [](unsigned x, unsigned y) { return sf::Vector2u{x, y}; };
static auto sz = [](unsigned w, unsigned h) { return sf::Vector2u{w, h}; };

namespace {

void
testEmptyTransparent()
{
    sf::Image img(sz(16, 16), sf::Color::Transparent);

    const auto rects = detectSpriteRects(img);
    SFMX_CHECK(rects.empty());
}

void
testSingleOpaquePixel()
{
    sf::Image img(sz(16, 16), sf::Color::Transparent);
    img.setPixel(uv(5, 7), sf::Color::White);

    const auto rects = detectSpriteRects(img);
    SFMX_CHECK(rects.size() == 1);
    if (rects.size() == 1) {
        SFMX_CHECK(rects[0].position.x == 5);
        SFMX_CHECK(rects[0].position.y == 7);
        SFMX_CHECK(rects[0].size.x == 1);
        SFMX_CHECK(rects[0].size.y == 1);
    }
}

void
testTwoDisjointSquares()
{
    sf::Image img(sz(32, 32), sf::Color::Transparent);

    for (int y = 2; y < 8; ++y)
        for (int x = 3; x < 9; ++x)
            img.setPixel(uv(x, y), sf::Color::White);

    for (int y = 20; y < 26; ++y)
        for (int x = 22; x < 28; ++x)
            img.setPixel(uv(x, y), sf::Color::White);

    const auto rects = detectSpriteRects(img);
    SFMX_CHECK(rects.size() == 2);
    if (rects.size() == 2) {
        SFMX_CHECK(rects[0].position.x == 3);
        SFMX_CHECK(rects[0].position.y == 2);
        SFMX_CHECK(rects[0].size.x == 6);
        SFMX_CHECK(rects[0].size.y == 6);

        SFMX_CHECK(rects[1].position.x == 22);
        SFMX_CHECK(rects[1].position.y == 20);
        SFMX_CHECK(rects[1].size.x == 6);
        SFMX_CHECK(rects[1].size.y == 6);
    }
}

void
testLShapeOneBoundingRect()
{
    sf::Image img(sz(16, 16), sf::Color::Transparent);

    for (int x = 2; x < 10; ++x)
        img.setPixel(uv(x, 2), sf::Color::White);
    for (int y = 2; y < 10; ++y)
        img.setPixel(uv(2, y), sf::Color::White);

    const auto rects = detectSpriteRects(img);
    SFMX_CHECK(rects.size() == 1);
    if (rects.size() == 1) {
        SFMX_CHECK(rects[0].position.x == 2);
        SFMX_CHECK(rects[0].position.y == 2);
        SFMX_CHECK(rects[0].size.x == 8);
        SFMX_CHECK(rects[0].size.y == 8);
    }
}

void
testDiagonalTouchSeparate()
{
    sf::Image img(sz(16, 16), sf::Color::Transparent);

    img.setPixel(uv(4, 4), sf::Color::White);
    img.setPixel(uv(5, 5), sf::Color::White);

    const auto rects = detectSpriteRects(img);
    SFMX_CHECK(rects.size() == 2);
}

void
testPaddingExpandsRects()
{
    sf::Image img(sz(16, 16), sf::Color::Transparent);
    img.setPixel(uv(7, 7), sf::Color::White);

    const auto rects = detectSpriteRects(img, 1, 2);
    SFMX_CHECK(rects.size() == 1);
    if (rects.size() == 1) {
        SFMX_CHECK(rects[0].position.x == 5);
        SFMX_CHECK(rects[0].position.y == 5);
        SFMX_CHECK(rects[0].size.x == 5);
        SFMX_CHECK(rects[0].size.y == 5);
    }
}

void
testPaddingClampedToBounds()
{
    sf::Image img(sz(8, 8), sf::Color::Transparent);
    img.setPixel(uv(0, 0), sf::Color::White);

    const auto rects = detectSpriteRects(img, 1, 10);
    SFMX_CHECK(rects.size() == 1);
    if (rects.size() == 1) {
        SFMX_CHECK(rects[0].position.x == 0);
        SFMX_CHECK(rects[0].position.y == 0);
        SFMX_CHECK(rects[0].size.x == 8);
        SFMX_CHECK(rects[0].size.y == 8);
    }
}

void
testAlphaThreshold()
{
    sf::Image img(sz(8, 8), sf::Color::Transparent);

    img.setPixel(uv(2, 2), sf::Color(255, 255, 255, 3));
    img.setPixel(uv(5, 5), sf::Color::White);

    const auto rectsLow = detectSpriteRects(img, 1);
    SFMX_CHECK(rectsLow.size() == 2);

    const auto rectsHigh = detectSpriteRects(img, 5);
    SFMX_CHECK(rectsHigh.size() == 1);
    if (rectsHigh.size() == 1) {
        SFMX_CHECK(rectsHigh[0].position.x == 5);
        SFMX_CHECK(rectsHigh[0].position.y == 5);
    }
}

void
testSortedTopToBottom()
{
    sf::Image img(sz(16, 16), sf::Color::Transparent);

    img.setPixel(uv(4, 10), sf::Color::White);
    img.setPixel(uv(4, 2), sf::Color::White);

    const auto rects = detectSpriteRects(img);
    SFMX_CHECK(rects.size() == 2);
    if (rects.size() == 2) {
        SFMX_CHECK(rects[0].position.y == 2);
        SFMX_CHECK(rects[1].position.y == 10);
    }
}

void
testZeroSizeImage()
{
    sf::Image img;  // default-constructed: 0x0

    const auto rects = detectSpriteRects(img);
    SFMX_CHECK(rects.empty());
}

} // namespace

// ---------------------------------------------------------------------------
void
runSpriteAtlasTests() {
    const int failedBefore = sfmxtest::g_checksFailed;

    testEmptyTransparent();
    testSingleOpaquePixel();
    testTwoDisjointSquares();
    testLShapeOneBoundingRect();
    testDiagonalTouchSeparate();
    testPaddingExpandsRects();
    testPaddingClampedToBounds();
    testAlphaThreshold();
    testSortedTopToBottom();
    testZeroSizeImage();

    if (sfmxtest::g_checksFailed == failedBefore) {
        std::cout << "[SpriteAtlasTest] passed\n";
    }
}
