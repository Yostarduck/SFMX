#include "scene/AnimatorComponent.h"
#include "scene/SceneNode.h"
#include "utils/Arithmetic.h"
#include "resource/Frame.h"

namespace sfmx {

AnimatorComponent::AnimatorComponent(SceneNode* owner)
: ComponentT<AnimatorComponent>(owner) {
  if (!m_owner->getComponent<SpriteComponent>()) {
    m_owner->addComponent<SpriteComponent>();
  }
  setSprite(m_owner->getComponent<SpriteComponent>());
}

AnimatorComponent::~AnimatorComponent() {
  for (auto& [_, node] : m_animations) {
    for (auto* t : node->transitions) {
      delete t;
    }
    delete node;
  }
}

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
    m_currentAnimation = it->second;
    if (!m_currentAnimation->animation->m_frames.empty()) {
      m_sprite->setFrame(m_currentAnimation->animation->m_frames[0]);
    }
  }
}

void
AnimatorComponent::addAnimation(Animation* newAnimation, const String& name) {
  String newName = name;
  if (m_animations.count(newName)) {
    newName += (name + std::to_string(m_animations.count(name)));
  }
  AnimationNode* newNode = new AnimationNode();
  newNode->animation = newAnimation;
  m_animations.try_emplace(newName, newNode);
}

void
AnimatorComponent::addAnimation(AnimationNode* newAnimation, const String& name) {
  String newName = name;
  if (m_animations.count(newName)) {
    newName += (name + std::to_string(m_animations.count(name)));
  }
  m_animations.try_emplace(newName, newAnimation);
}

void
AnimatorComponent::addTransition(const String& fromAnim, const String& toAnim,
                                  bool hasExitTime, bool shouldTransition,
                                  const Map<String, Param>& conditions) {
  auto it = m_animations.find(fromAnim);
  if (it == m_animations.end()) return;

  AnimationTransition* t = new AnimationTransition();
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

  if (it->second == m_currentAnimation) {
    stop();
    m_currentAnimation = nullptr;
  }
  for (auto* t : it->second->transitions) {
    delete t;
  }
  delete it->second;
  m_animations.erase(it);
}

void
AnimatorComponent::onUpdate(float deltaTime) {
  checkAnimationState(deltaTime);
  updateParamTriggers();
  if (m_state != AnimationState::kStarted) return;
  if (!m_sprite || !m_currentAnimation) return;

  Animation* a = m_currentAnimation->animation;
  float frame = (a->m_duration > 0.f) ?
                (m_currentTime / a->m_duration) * a->m_frames.size() : 0.f;
  size_t idx = std::min(static_cast<size_t>(frame), a->m_frames.size() - 1);
  m_sprite->setFrame(a->m_frames[idx]);
  m_currentTime += deltaTime * a->m_speedMultiplier;
}

void
AnimatorComponent::checkAnimationState(float /*deltaTime*/) {
  if (!m_currentAnimation) {
    m_state = AnimationState::kStopped;
    return;
  }
  if (m_state != AnimationState::kStarted) return;

  Animation* a = m_currentAnimation->animation;

  for (const auto& t : m_currentAnimation->transitions) {
    if (!t->shouldTransition) continue;
    if (t->hasExitTime && m_currentTime < a->m_duration) continue;
    if (!evaluateTransitionConditions(t)) continue;
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

  m_currentAnimation = it->second;
  m_currentTime = 0.0f;
  m_state = AnimationState::kStarted;
  if (!m_currentAnimation->animation->m_frames.empty()) {
    m_sprite->setFrame(m_currentAnimation->animation->m_frames[0]);
  }
}

bool
AnimatorComponent::evaluateTransitionConditions(const AnimationTransition* t) const {
  for (const auto& [key, required] : t->params) {
    auto it = m_params.find(key);
    if (it == m_params.end()) return false;
    if (required.t != it->second.t) return false;

    if (required.t == ParamType::kTrigger) {
      if (!std::any_cast<bool>(it->second.v)) return false;
    } else if (required.t == ParamType::kBool) {
      if (std::any_cast<bool>(required.v) != std::any_cast<bool>(it->second.v)) return false;
    } else if (required.t == ParamType::kFloat) {
      if (std::any_cast<float>(required.v) != std::any_cast<float>(it->second.v)) return false;
    } else if (required.t == ParamType::kInt) {
      if (std::any_cast<int>(required.v) != std::any_cast<int>(it->second.v)) return false;
    }
  }
  return true;
}

void
AnimatorComponent::updateParamTriggers() {
  for (auto& p : m_params) {
    if (ParamType::kTrigger == p.second.t) {
      p.second.v = false;
    }
  }
}

} // namespace sfmx
