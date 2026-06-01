#include "test.h"

namespace sfmx
{
TestComponent::TestComponent()
: m_circle(50.f)
{
  m_circle.setFillColor(sf::Color(100, 180, 255));
  m_circle.setPosition({320.f, 220.f});
}

void TestComponent::draw(sf::RenderTarget& target) const
{
  target.draw(m_circle);
}
}
