#include "resource/SpriteAtlas.h"

#include <algorithm>

namespace sfmx
{

namespace
{

struct Coord
{
    int32 x, y;
};

void floodFill(const sf::Image& image,
               int32 startX, int32 startY,
               uint8 alphaThreshold,
               Vector<uint8>& visited,
               int32 w, int32 h,
               int32& outMinX, int32& outMaxX,
               int32& outMinY, int32& outMaxY)
{
    Vector<Coord> stack;
    stack.push_back({startX, startY});

    outMinX = startX; outMaxX = startX;
    outMinY = startY; outMaxY = startY;

    while (!stack.empty())
    {
        Coord c = stack.back();
        stack.pop_back();

        if (c.x < 0 || c.x >= w || c.y < 0 || c.y >= h)
            continue;

        const size_t idx = static_cast<size_t>(c.y) * static_cast<size_t>(w)
                         + static_cast<size_t>(c.x);
        if (visited[idx])
            continue;

        const sf::Color pixel = image.getPixel(
            sf::Vector2u(static_cast<unsigned>(c.x), static_cast<unsigned>(c.y)));
        if (pixel.a <= alphaThreshold)
        {
            visited[idx] = 1;
            continue;
        }

        visited[idx] = 1;

        if (c.x < outMinX) outMinX = c.x;
        if (c.x > outMaxX) outMaxX = c.x;
        if (c.y < outMinY) outMinY = c.y;
        if (c.y > outMaxY) outMaxY = c.y;

        stack.push_back({c.x - 1, c.y});
        stack.push_back({c.x + 1, c.y});
        stack.push_back({c.x, c.y - 1});
        stack.push_back({c.x, c.y + 1});
    }
}

} // namespace

Vector<sf::IntRect>
detectSpriteRects(const sf::Image& image,
                  uint8 alphaThreshold,
                  int32 padding)
{
    const int32 w = static_cast<int32>(image.getSize().x);
    const int32 h = static_cast<int32>(image.getSize().y);

    if (w == 0 || h == 0)
        return {};

    Vector<uint8> visited(static_cast<size_t>(w) * static_cast<size_t>(h), 0);
    Vector<sf::IntRect> result;

    for (int32 y = 0; y < h; ++y)
    {
        for (int32 x = 0; x < w; ++x)
        {
            const size_t idx = static_cast<size_t>(y) * static_cast<size_t>(w)
                             + static_cast<size_t>(x);
            if (visited[idx])
                continue;

            const sf::Color pixel = image.getPixel(
                sf::Vector2u(static_cast<unsigned>(x), static_cast<unsigned>(y)));
            if (pixel.a <= alphaThreshold)
            {
                visited[idx] = 1;
                continue;
            }

            int32 minX, maxX, minY, maxY;
            floodFill(image, x, y, alphaThreshold, visited, w, h,
                      minX, maxX, minY, maxY);

            const int32 left   = std::max(minX - padding, 0);
            const int32 top    = std::max(minY - padding, 0);
            const int32 right  = std::min(maxX + padding, w - 1);
            const int32 bottom = std::min(maxY + padding, h - 1);

            result.push_back({{left, top},
                              {right - left + 1, bottom - top + 1}});
        }
    }

    std::sort(result.begin(), result.end(),
              [](const sf::IntRect& a, const sf::IntRect& b)
              {
                  if (a.position.y != b.position.y)
                      return a.position.y < b.position.y;
                  return a.position.x < b.position.x;
              });

    return result;
}

} // namespace sfmx
