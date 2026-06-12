#pragma once

#include "core/platform/Prerequisites.h"

#include <SFML/Graphics/Color.hpp>

namespace sfmx
{
class Frame;

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