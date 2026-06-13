#pragma once

#include "core/platform/Prerequisites.h"

#include <SFML/Graphics/Color.hpp>

namespace sfmx
{
class Frame;

struct Animation
{
  Vector<Frame>  m_frames;
  Vector<float>  m_frameDurations;  // Per-frame durations (empty = even split)
  bool           m_loops            = false;
  float          m_speedMultiplier  = 1.0f;
  float          m_duration         = 0.0f;
};

}
