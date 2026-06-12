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

  if (m_animations.find(animation) != m_animations.end()) {
    stop();
    m_currentAnimation = m_animations.find(animation)->second;
  }
  // TODO: Show user there was no animation with that name
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
AnimatorComponent::removeAnimation(const String& name) {
  if (!animationExists(name)) {
    return;
  }

  if(m_animations.find(name)->second == m_currentAnimation) {
    stop();
    m_currentAnimation = nullptr;
  }
  m_animations.erase(name);
}

void
AnimatorComponent::onUpdate(float deltaTime) {
  
  checkAnimationState(deltaTime);
  if (AnimationState::kPaused == m_state || 
      AnimationState::kStopped == m_state ) {
    return;
  }
  if (m_sprite && m_currentAnimation) {
    
    Animation* a = m_currentAnimation->animation;
    float frame = (a->m_duration > 0.f) ? 
                  (m_currentTime / a->m_duration) * a->m_frames.size() : 0.f;
    Frame f = a->m_frames.at(static_cast<size_t>(frame));
    m_sprite->setFrame(f);
    m_currentTime += (deltaTime * m_currentAnimation->animation->m_speedMultiplier);
  }

}

void
AnimatorComponent::checkAnimationState(float deltaTime) {
  if (!m_currentAnimation) {
    m_state = AnimationState::kStopped;
    return;
  }
  if (AnimationState::kStarted == m_state ) {
    if (m_currentTime >= m_currentAnimation->animation->m_duration) {
      if (!m_currentAnimation->animation->m_loops) {
        m_state = AnimationState::kStopped;
      }
      m_currentTime = 0.0f;
      
    }
  }
}


}