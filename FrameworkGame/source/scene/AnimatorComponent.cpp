#include "scene/AnimatorComponent.h"
#include "scene/SceneNode.h"
#include "utils/Arithmetic.h"
#include "resource/Frame.h"

#include "assets/AssetManager.h"
#include "assets/TextureAsset.h"
#include "core/DataStream.h"
#include "core/DataStreamTypes.h"

namespace sfmx {
namespace {

constexpr uint32 kAnimatorComponentVersion = 1;

size_t computeFrameIndex(const Animation& anim, float currentTime) {
  if (anim.m_frames.empty()) return 0;

  const auto& durations = anim.m_frameDurations;
  if (!durations.empty()) {
    float accum = 0.0f;
    for (size_t i = 0; i < durations.size(); ++i) {
      accum += durations[i];
      if (currentTime < accum) return i;
    }
    return durations.size() - 1;
  }

  if (anim.m_duration > 0.f) {
    float frame = (currentTime / anim.m_duration) * static_cast<float>(anim.m_frames.size());
    return std::min(static_cast<size_t>(frame), anim.m_frames.size() - 1);
  }
  return 0;
}

// --- Serialization helpers (Map/Variant have no stream overloads, so they are
//     hand-rolled: count + pairs; the ParamType tag discriminates the variant). ---

void
writeParam(DataStream& stream, const Param& param) {
  stream << static_cast<uint8>(param.type);
  switch (param.type) {
    case ParamType::kBool:
    case ParamType::kTrigger: {
      const bool* v = std::get_if<bool>(&param.value);
      stream << static_cast<uint8>((nullptr != v && *v) ? 1 : 0);
      break;
    }
    case ParamType::kFloat: {
      const float* v = std::get_if<float>(&param.value);
      stream << ((nullptr != v) ? *v : 0.0f);
      break;
    }
    case ParamType::kInt: {
      const int* v = std::get_if<int>(&param.value);
      stream << static_cast<int32>((nullptr != v) ? *v : 0);
      break;
    }
    default:
      break;
  }
}

Param
readParam(DataStream& stream) {
  uint8 typeTag = 0;
  stream >> typeTag;
  Param param;
  param.type = static_cast<ParamType>(typeTag);
  switch (param.type) {
    case ParamType::kBool:
    case ParamType::kTrigger: {
      uint8 v = 0;
      stream >> v;
      param.value = (v != 0);
      break;
    }
    case ParamType::kFloat: {
      float v = 0.0f;
      stream >> v;
      param.value = v;
      break;
    }
    case ParamType::kInt: {
      int32 v = 0;
      stream >> v;
      param.value = static_cast<int>(v);
      break;
    }
    default:
      param.value = false;
      break;
  }
  return param;
}

void
writeParamMap(DataStream& stream, const Map<String, Param>& params) {
  stream << static_cast<uint64>(params.size());
  for (const auto& [name, param] : params) {
    stream.writeString(name);
    writeParam(stream, param);
  }
}

void
readParamMap(DataStream& stream, Map<String, Param>& out) {
  out.clear();
  uint64 count = 0;
  stream >> count;
  for (uint64 i = 0; i < count; ++i) {
    String name = stream.readString();
    out[name] = readParam(stream);
  }
}

} // namespace

AnimatorComponent::AnimatorComponent(SceneNode* owner)
: ComponentT<AnimatorComponent>(owner) {
  if (!m_owner->getComponent<SpriteComponent>()) {
    m_owner->addComponent<SpriteComponent>();
  }
  setSprite(m_owner->getComponent<SpriteComponent>());
}

AnimatorComponent::~AnimatorComponent() = default;

void
AnimatorComponent::play(const String& animation) {
  setCurrentAnimation(animation);
  play();
}

void
AnimatorComponent::stop() {
  m_state = AnimationState::kStopped;
  m_currentTime = 0.0f;
}

void
AnimatorComponent::setCurrentAnimation(const String& animation) {
  if (!animationExists(animation)) {
    return;
  }

  auto it = m_animations.find(animation);
  stop();
  m_currentAnimation = it->second.get();
  if (!m_currentAnimation->animation->m_frames.empty()) {
    m_sprite->setFrame(m_currentAnimation->animation->m_frames[0]);
  }
}

void
AnimatorComponent::addAnimation(SPtr<Animation> newAnimation, const String& name) {
  String newName = name;
  if (m_animations.count(newName)) {
    newName += (name + std::to_string(m_animations.count(name)));
  }
  auto newNode = MakeUnique<AnimationNode>();
  newNode->animation = newAnimation;
  m_animations.try_emplace(newName, std::move(newNode));
}

void
AnimatorComponent::addAnimation(UniquePtr<AnimationNode> newAnimation, const String& name) {
  String newName = name;
  if (m_animations.count(newName)) {
    newName += (name + std::to_string(m_animations.count(name)));
  }
  m_animations.try_emplace(newName, std::move(newAnimation));
}

void
AnimatorComponent::addTransition(const String& fromAnim, const String& toAnim,
                                  bool hasExitTime, bool shouldTransition,
                                  const Map<String, Param>& conditions) {
  auto itBeg = m_animations.find(fromAnim);
  auto itEnd = m_animations.find(toAnim);
  if (itBeg == m_animations.end() || itBeg == m_animations.end()) return;

  auto t = MakeShared<AnimationTransition>();
  t->exit = toAnim;
  t->hasExitTime = hasExitTime;
  t->shouldTransition = shouldTransition;
  t->params = conditions;
  itBeg->second->transitions.push_back(t);
}

void
AnimatorComponent::removeAnimation(const String& name) {
  auto it = m_animations.find(name);
  if (it == m_animations.end()) return;

  if (it->second.get() == m_currentAnimation) {
    stop();
    m_currentAnimation = nullptr;
  }
  m_animations.erase(it);
}

void
AnimatorComponent::onUpdate(float deltaTime) {
  checkAnimationState(deltaTime);
  updateParamTriggers();
  if (m_state != AnimationState::kPlaying) return;
  if (!m_sprite || !m_currentAnimation) return;

  Animation* a = m_currentAnimation->animation.get();
  size_t idx = computeFrameIndex(*a, m_currentTime);
  m_sprite->setFrame(a->m_frames[idx]);
  m_currentTime += deltaTime * a->m_speedMultiplier;
}

void
AnimatorComponent::checkAnimationState(float /*deltaTime*/) {
  if (!m_currentAnimation) {
    m_state = AnimationState::kStopped;
    return;
  }
  if (m_state != AnimationState::kPlaying) return;

  Animation* a = m_currentAnimation->animation.get();

  for (const auto& t : m_currentAnimation->transitions) {
    if (!t->shouldTransition) continue;
    if (t->hasExitTime && m_currentTime < a->m_duration) continue;
    if (!evaluateTransitionConditions(t.get())) continue;
    transitionTo(t->exit);
    return;
  }

  if (m_currentTime >= a->m_duration) {
    if (!a->m_loops) {
      m_state = AnimationState::kStopped;
    }
    m_currentTime = 0.0f;
  }
}

void
AnimatorComponent::transitionTo(const String& name) {
  auto it = m_animations.find(name);
  if (it == m_animations.end()) return;

  m_currentAnimation = it->second.get();
  m_currentTime = 0.0f;
  m_state = AnimationState::kPlaying;
  if (!m_currentAnimation->animation->m_frames.empty()) {
    m_sprite->setFrame(m_currentAnimation->animation->m_frames[0]);
  }
}

bool
AnimatorComponent::evaluateTransitionConditions(const AnimationTransition* t) const {
  for (const auto& [key, required] : t->params) {
    auto it = m_params.find(key);
    if (it == m_params.end()) return false;
    if (required.type != it->second.type) return false;

    if (required.type == ParamType::kTrigger) {
      const auto* v = std::get_if<bool>(&it->second.value);
      if (!v || !*v) return false;
    } else if (required.type == ParamType::kBool) {
      const auto* reqV = std::get_if<bool>(&required.value);
      const auto* curV = std::get_if<bool>(&it->second.value);
      if (!reqV || !curV || *reqV != *curV) return false;
    } else if (required.type == ParamType::kFloat) {
      const auto* reqV = std::get_if<float>(&required.value);
      const auto* curV = std::get_if<float>(&it->second.value);
      if (!reqV || !curV || *reqV != *curV) return false;
    } else if (required.type == ParamType::kInt) {
      const auto* reqV = std::get_if<int>(&required.value);
      const auto* curV = std::get_if<int>(&it->second.value);
      if (!reqV || !curV || *reqV != *curV) return false;
    }
  }
  return true;
}

void
AnimatorComponent::updateParamTriggers() {
  for (auto& p : m_params) {
    if (ParamType::kTrigger == p.second.type) {
      p.second.value = false;
    }
  }
}

// -----------------------------------------------------------------------------
// Serialization
// -----------------------------------------------------------------------------

void
AnimatorComponent::onSerialize(DataStream& stream) const {
  stream << kAnimatorComponentVersion;

  stream << static_cast<uint64>(m_animations.size());
  for (const auto& [name, node] : m_animations) {
    stream.writeString(name);

    const Animation* anim = node->animation.get();
    const uint8 hasAnim = (nullptr != anim) ? 1 : 0;
    stream << hasAnim;
    if (hasAnim) {
      stream << static_cast<uint8>(anim->m_loops ? 1 : 0);
      stream << anim->m_speedMultiplier;
      stream << anim->m_duration;
      stream << anim->m_textureAssetId;
      stream << anim->m_frameDurations;  // Vector<float> bulk overload

      stream << static_cast<uint64>(anim->m_frames.size());
      for (const Frame& frame : anim->m_frames) {
        stream << frame.framing.position.x << frame.framing.position.y;
        stream << frame.framing.size.x << frame.framing.size.y;
        stream << frame.scale.x << frame.scale.y;
        stream << frame.position.x << frame.position.y;
        stream << frame.rotation.asRadians();
        stream << frame.color.r << frame.color.g << frame.color.b << frame.color.a;
        stream << static_cast<uint8>(frame.flippedX ? 1 : 0);
        stream << static_cast<uint8>(frame.flippedY ? 1 : 0);
      }
    }

    stream << static_cast<uint64>(node->transitions.size());
    for (const SPtr<AnimationTransition>& t : node->transitions) {
      stream.writeString(t->exit);
      stream << static_cast<uint8>(t->hasExitTime ? 1 : 0);
      stream << static_cast<uint8>(t->shouldTransition ? 1 : 0);
      writeParamMap(stream, t->params);
    }
  }

  writeParamMap(stream, m_params);

  // Selected clip by name (reverse pointer→key lookup); empty if none selected.
  String currentName;
  if (nullptr != m_currentAnimation) {
    for (const auto& [name, node] : m_animations) {
      if (node.get() == m_currentAnimation) {
        currentName = name;
        break;
      }
    }
  }
  stream.writeString(currentName);
}

void
AnimatorComponent::onDeserialize(DataStream& stream) {
  uint32 version = 0;
  stream >> version;
  if (version != kAnimatorComponentVersion) {
    return;
  }

  m_animations.clear();
  m_currentAnimation = nullptr;
  m_currentTime = 0.0f;
  m_state = AnimationState::kStopped;

  uint64 animCount = 0;
  stream >> animCount;
  for (uint64 i = 0; i < animCount; ++i) {
    String name = stream.readString();

    auto animNode = MakeUnique<AnimationNode>();

    uint8 hasAnim = 0;
    stream >> hasAnim;
    if (hasAnim) {
      auto anim = MakeShared<Animation>();

      uint8 loops = 0;
      stream >> loops;
      anim->m_loops = (loops != 0);
      stream >> anim->m_speedMultiplier;
      stream >> anim->m_duration;
      stream >> anim->m_textureAssetId;
      stream >> anim->m_frameDurations;  // Vector<float> bulk overload

      // Resolve the shared atlas once; frames hold aliasing SPtrs onto it.
      SPtr<sf::Texture> atlas;
      if (anim->m_textureAssetId != UUID::null() && AssetManager::isStarted()) {
        SPtr<TextureAsset> asset =
            AssetManager::instance().load<TextureAsset>(anim->m_textureAssetId);
        if (nullptr != asset && asset->isLoaded()) {
          atlas = SPtr<sf::Texture>(asset, &asset->texture());
        }
      }

      uint64 frameCount = 0;
      stream >> frameCount;
      anim->m_frames.reserve(static_cast<size_t>(frameCount));
      for (uint64 f = 0; f < frameCount; ++f) {
        Frame frame;
        int32 px = 0, py = 0, sx = 0, sy = 0;
        stream >> px >> py >> sx >> sy;
        frame.framing = sf::IntRect({px, py}, {sx, sy});
        stream >> frame.scale.x >> frame.scale.y;
        stream >> frame.position.x >> frame.position.y;
        float radians = 0.0f;
        stream >> radians;
        frame.rotation = sf::radians(radians);
        stream >> frame.color.r >> frame.color.g >> frame.color.b >> frame.color.a;
        uint8 flipX = 0;
        uint8 flipY = 0;
        stream >> flipX >> flipY;
        frame.flippedX = (flipX != 0);
        frame.flippedY = (flipY != 0);
        frame.texture = atlas;
        anim->m_frames.push_back(std::move(frame));
      }

      animNode->animation = anim;
    }

    uint64 transitionCount = 0;
    stream >> transitionCount;
    for (uint64 t = 0; t < transitionCount; ++t) {
      auto transition = MakeShared<AnimationTransition>();
      transition->exit = stream.readString();
      uint8 hasExitTime = 0;
      uint8 shouldTransition = 0;
      stream >> hasExitTime >> shouldTransition;
      transition->hasExitTime = (hasExitTime != 0);
      transition->shouldTransition = (shouldTransition != 0);
      readParamMap(stream, transition->params);
      animNode->transitions.push_back(transition);
    }

    m_animations.try_emplace(name, std::move(animNode));
  }

  readParamMap(stream, m_params);

  // Re-bind to the owner's sprite (the ctor already did, but stay defensive).
  setSprite(m_owner->getComponent<SpriteComponent>());

  // Re-select the clip, leaving the animator stopped at t=0 (no auto-play).
  String currentName = stream.readString();
  if (!currentName.empty()) {
    setCurrentAnimation(currentName);
  }
}

} // namespace sfmx
