#include "input/Processor.h"

#include <algorithm>
#include <cmath>

namespace sfmx
{

namespace {

float
magnitudeOf(Vector2f value, ActionValueType::E type) {
  if (ActionValueType::kAxis2D == type) {
    return std::sqrt(value.x * value.x + value.y * value.y);
  }
  return std::fabs(value.x);
}

Vector2f
applyOne(const Processor& processor, Vector2f value, ActionValueType::E type) {
  switch (processor.m_type) {
    case ProcessorType::kInvert: {
      if (0.f != processor.m_paramA.x) { value.x = -value.x; }
      if (0.f != processor.m_paramA.y) { value.y = -value.y; }
      return value;
    }
    case ProcessorType::kScale: {
      value.x *= processor.m_paramA.x;
      value.y *= processor.m_paramA.y;
      return value;
    }
    case ProcessorType::kDeadZone: {
      const float deadMin = processor.m_paramA.x;
      const float deadMax = processor.m_paramA.y > deadMin ? processor.m_paramA.y : 1.f;
      const float magnitude = magnitudeOf(value, type);
      if (magnitude <= deadMin) {
        return Vector2f{0.f, 0.f};
      }
      const float span = deadMax - deadMin;
      const float scaled = std::clamp((magnitude - deadMin) / span, 0.f, 1.f);
      const float factor = scaled / magnitude;  // rescale, preserving direction
      return Vector2f{value.x * factor, value.y * factor};
    }
    case ProcessorType::kNormalize: {
      if (ActionValueType::kAxis2D != type) {
        return value;
      }
      const float magnitude = magnitudeOf(value, type);
      if (magnitude > 1.f) {
        return Vector2f{value.x / magnitude, value.y / magnitude};
      }
      return value;
    }
    case ProcessorType::kClamp: {
      value.x = std::clamp(value.x, processor.m_paramA.x, processor.m_paramB.x);
      value.y = std::clamp(value.y, processor.m_paramA.y, processor.m_paramB.y);
      return value;
    }
    default:
      return value;
  }
}

} // namespace

Vector2f
applyProcessors(const Vector<Processor>& processors,
                Vector2f value,
                ActionValueType::E type) {
  for (const Processor& processor : processors) {
    value = applyOne(processor, value, type);
  }
  return value;
}

} // namespace sfmx
