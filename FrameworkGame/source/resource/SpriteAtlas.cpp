#include "resource/SpriteAtlas.h"

#include <algorithm>

namespace sfmx::Atlas
{

namespace
{

using Coord = sf::Vector2i;

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

  outMinX = outMaxX = startX;
  outMinY = outMaxY = startY;

  while (!stack.empty())
  {
    Coord c = stack.back();
    stack.pop_back();

    const int32 x = c.x;
    const int32 y = c.y;

    if (x < 0 || x >= w || y < 0 || y >= h)
      continue;

    const size_t idx = static_cast<size_t>(y) * static_cast<size_t>(w) + static_cast<size_t>(x);

    if (visited[idx])
      continue;
    visited[idx] = 1;

    const sf::Color pixel = image.getPixel(
        sf::Vector2u(static_cast<unsigned>(x), static_cast<unsigned>(y)));
    if (pixel.a <= alphaThreshold)
      continue;

    if (x < outMinX) outMinX = x;
    if (x > outMaxX) outMaxX = x;
    if (y < outMinY) outMinY = y;
    if (y > outMaxY) outMaxY = y;

    stack.push_back({x - 1, y});
    stack.push_back({x + 1, y});
    stack.push_back({x, y - 1});
    stack.push_back({x, y + 1});
  }
}

} // anonymous namespace

Vector<sf::IntRect>
detectSpriteRects(const sf::Image& image,
                  uint8 alphaThreshold,
                  int32 padding)
{
  Vector<sf::IntRect> result;

  const sf::Vector2u size = image.getSize();
  const int32 w = static_cast<int32>(size.x);
  const int32 h = static_cast<int32>(size.y);

  if (w <= 0 || h <= 0)
    return result;

  Vector<uint8> visited(static_cast<size_t>(w) * static_cast<size_t>(h), 0);

  // Row-major scan: top-to-bottom, left-to-right.
  for (int32 y = 0; y < h; ++y)
  {
    for (int32 x = 0; x < w; ++x)
    {
      const size_t idx = static_cast<size_t>(y) * static_cast<size_t>(w) + static_cast<size_t>(x);

      if (visited[idx])
        continue;

      const sf::Color pixel = image.getPixel(sf::Vector2u(static_cast<unsigned>(x),
                                                           static_cast<unsigned>(y)));

      if (pixel.a <= alphaThreshold)
        continue;

      // New sprite region found — flood-fill to find its bounding box.
      int32 minX, maxX, minY, maxY;
      floodFill(image, x, y, alphaThreshold, visited, w, h,
                minX, maxX, minY, maxY);

      // Clamp padding so the rect stays inside the image.
      int32 left   = std::max(0, minX - padding);
      int32 top    = std::max(0, minY - padding);
      int32 right  = std::min(w - 1, maxX + padding);
      int32 bottom = std::min(h - 1, maxY + padding);

      const int32 rw = right - left + 1;
      const int32 rh = bottom - top + 1;

      if (rw > 0 && rh > 0)
        result.push_back({{left, top}, {rw, rh}});
    }
  }

  // Sort: top-to-bottom, then left-to-right.
  std::sort(result.begin(), result.end(),
            [](const sf::IntRect& a, const sf::IntRect& b)
            {
              if (a.position.y != b.position.y)
                return a.position.y < b.position.y;
              return a.position.x < b.position.x;
            });

  return result;
}

Vector<sf::IntRect>
getSpriteRectsByGrid(const sf::Vector2u& atlasSize, uint32 columns, uint32 rows)
{
  Vector<sf::IntRect> result;
  if (columns == 0 || rows == 0)
    return result;

  const int32 cellW = static_cast<int32>(atlasSize.x / columns);
  const int32 cellH = static_cast<int32>(atlasSize.y / rows);

  for (uint32 r = 0; r < rows; ++r)
  {
    for (uint32 c = 0; c < columns; ++c)
    {
      result.push_back({
        {static_cast<int32>(c * cellW), static_cast<int32>(r * cellH)},
        {cellW, cellH}
      });
    }
  }
  return result;
}

Vector<sf::IntRect>
getSpriteRectsByRectSize(const sf::Vector2u& atlasSize,
                         uint32 spriteWidth, uint32 spriteHeight)
{
  Vector<sf::IntRect> result;
  if (spriteWidth == 0 || spriteHeight == 0)
    return result;

  const uint32 cols = (atlasSize.x + spriteWidth - 1) / spriteWidth;
  const uint32 rows = (atlasSize.y + spriteHeight - 1) / spriteHeight;

  for (uint32 r = 0; r < rows; ++r)
  {
    for (uint32 c = 0; c < cols; ++c)
    {
      const int32 x = static_cast<int32>(c * spriteWidth);
      const int32 y = static_cast<int32>(r * spriteHeight);
      const int32 w = std::min(static_cast<int32>(spriteWidth),
                                static_cast<int32>(atlasSize.x) - x);
      const int32 h = std::min(static_cast<int32>(spriteHeight),
                                static_cast<int32>(atlasSize.y) - y);
      if (w > 0 && h > 0)
        result.push_back({{x, y}, {w, h}});
    }
  }
  return result;
}

} // namespace sfmx::Atlas
