#pragma once

#include "core/platform/Prerequisites.h"
#include "utils/UUID.h"

#include <SFML/Graphics/Color.hpp>

namespace sfmx
{
class Frame;

struct Animation
{
  Vector<Frame>  m_frames;
  Vector<float>  m_frameDurations;
  bool           m_loops            = false;
  float          m_speedMultiplier  = 1.0f;
  float          m_duration         = 0.0f;
  //!< Serializable handle to the shared atlas TextureAsset (null for raw textures).
  UUID           m_textureAssetId   = UUID::null();
};

}
