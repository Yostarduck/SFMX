#pragma once
#include "core/platform/Prerequisites.h"

#include "scene/Component.h"
#include "scene/SpriteComponent.h"
#include "scene/Animation.h"


namespace sfmx {
// Forward declaration
// class   SpriteComponent;
// class   Animation;
struct  AnimationNode;
struct  AnimationTransition;
} // namespace sfmx

namespace sfmx {

struct AnimationNode
{
  Animation*                    animation   = nullptr;
  Vector<AnimationTransition*>  transitions = {};
};

enum class AnimationState : uint32
{
  kStarted, kPaused, kStopped
};

struct AnimationTransition
{
  // TODO: Add list of requirements for it to change. For now it this ok
  Animation*  exit              = nullptr;
  bool        hasExitTime       = true;
  bool        shouldTransition  = true;
  Map<String, std::any> params  = {}; 
};

class AnimatorComponent : public ComponentT<AnimatorComponent> {
 public:

  AnimatorComponent(SceneNode* owner);
  ~AnimatorComponent() override;


  void play(const String& animation);
  FORCEINLINE void 
  play() { m_state = AnimationState::kStarted; }
  FORCEINLINE void 
  pause() { m_state = AnimationState::kPaused; }
  void stop();

  void setCurrentAnimation(const String& animation);
  NODISCARD FORCEINLINE 
  AnimationNode* getCurrentAnimation(const String& animation) {
    return m_currentAnimation;
  }

  void 
  addAnimation(Animation* newAnimation, const String& name);
  void 
  addAnimation(AnimationNode* newAnimation, const String& name);
  void 
  removeAnimation(const String& name);

  NODISCARD FORCEINLINE bool 
  animationExists(const String& name) { return m_animations.count(name); }

  /** @brief Drives auto position from the node's world transform when m_followNode is true */
  void 
  onUpdate(float deltaTime) override;

  FORCEINLINE void 
  setSprite(SpriteComponent* newSprite) { m_sprite = newSprite; }

 private:

  void
  checkAnimationState(float deltaTime); 

  Map<String, AnimationNode*> m_animations;
  AnimationNode*              m_currentAnimation;
  Map<String, std::any>       m_params;
  SpriteComponent*  m_sprite;
  float             m_currentTime;
  AnimationState    m_state;

};

} // sfmx