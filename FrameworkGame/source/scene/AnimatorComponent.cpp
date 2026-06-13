#include "scene/AnimatorComponent.h"
#include "scene/SceneNode.h"
#include "utils/Arithmetic.h"
#include "resource/Frame.h"

namespace sfmx {
namespace {

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
  if (it != m_animations.end()) {
    stop();
    m_currentAnimation = it->second.get();
    if (!m_currentAnimation->animation->m_frames.empty()) {
      m_sprite->setFrame(m_currentAnimation->animation->m_frames[0]);
    }
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
  auto it = m_animations.find(fromAnim);
  if (it == m_animations.end()) return;

  auto t = MakeShared<AnimationTransition>();
  t->exit = toAnim;
  t->hasExitTime = hasExitTime;
  t->shouldTransition = shouldTransition;
  t->params = conditions;
  it->second->transitions.push_back(t);
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

} // namespace sfmx
