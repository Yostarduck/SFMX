#pragma once
#include "core/platform/Prerequisites.h"

#include "scene/Component.h"

namespace sfmx {
  // Forward declaration
  class   SpriteComponent;
  class   Animation;
  struct  AnimationNode;
  struct  AnimationTransition;
} // namespace sfmx

namespace sfmx {

struct AnimationNode
{
  Animation*                    animation   = nullptr;
  Vector<AnimationTransition*>  transitions = {};
};

struct AnimationTransition
{
  // TODO: Add list of requirements for it to change. For now it this ok
  Animation* exit = nullptr;
};

class AnimatorComponent : public ComponentT<AnimatorComponent> {
 public:

  AnimatorComponent(SceneNode* owner);
  ~AnimatorComponent() override;


  void play();
  void play(const String& animation);
  void pause();
  void stop();

  void setCurrentAnimation(const String& animation);
  NODISCARD AnimationNode* getCurrentAnimation(const String& animation);

  void addAnimation(Animation* newAnimation, const String& name);
  void addAnimation(AnimationNode* newAnimation, const String& name);
  
  void removeAnimation(const String& name);

  void setEntryAnimation(const String& animation);
  void setExitAnimation(const String& animation);

  /** @brief Drives auto position from the node's world transform when m_followNode is true */
  void onUpdate(float deltaTime) override;

  void setSprite(SpriteComponent* newSprite);

 private:

  Map<String, AnimationNode*> m_animations;
  AnimationNode*              m_currentAnimation;
  SpriteComponent*  m_sprite;
  float             m_currentTime;

};

} // sfmx