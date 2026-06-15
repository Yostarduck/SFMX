#pragma once
#include "core/platform/Prerequisites.h"

#include "scene/Component.h"
#include "scene/SpriteComponent.h"
#include "scene/Animation.h"

namespace sfmx {

enum class AnimationState : uint32
{
  kPlaying, kPaused, kStopped
};

enum class ParamType : uint32 {
  kBool,
  kFloat,
  kInt,
  kTrigger
};

struct Param {
  ParamType type;
  Variant<bool, float, int> value;
};

struct AnimationTransition {
  Map<String, Param>  params  = {};
  String              exit    = "";
  bool      shouldTransition  = true;
  bool      hasExitTime       = true;
};

struct AnimationNode
{
  SPtr<Animation>              animation   = nullptr;
  Vector<SPtr<AnimationTransition>>  transitions = {};
};

class AnimatorComponent : public ComponentT<AnimatorComponent> {
 public:

  AnimatorComponent(SceneNode* owner);
  ~AnimatorComponent() override;

  void play(const String& animation);
  FORCEINLINE void
  play() { m_state = AnimationState::kPlaying; }
  FORCEINLINE void
  pause() { m_state = AnimationState::kPaused; }
  void stop();

  void setCurrentAnimation(const String& animation);
  NODISCARD FORCEINLINE
  AnimationNode* getCurrentAnimation() const {
    return m_currentAnimation;
  }

  void
  addAnimation(SPtr<Animation> newAnimation, const String& name);
  void
  addAnimation(UniquePtr<AnimationNode> newAnimation, const String& name);
  void
  removeAnimation(const String& name);

  void
  addTransition(const String& fromAnim, const String& toAnim,
                bool hasExitTime = true, bool shouldTransition = true,
                const Map<String, Param>& conditions = {});

  NODISCARD FORCEINLINE bool
  animationExists(const String& name) const { return m_animations.count(name); }

  void
  onUpdate(float deltaTime) override;

  FORCEINLINE void
  setSprite(SpriteComponent* newSprite) { m_sprite = newSprite; }

  FORCEINLINE void
  setTrigger(const String& paramName) { m_params[paramName] = {ParamType::kTrigger, true}; }
  FORCEINLINE void
  setBool(const String& paramName, bool value) { m_params[paramName] = {ParamType::kBool, value}; }
  FORCEINLINE void
  setFloat(const String& paramName, float value) { m_params[paramName] = {ParamType::kFloat, value}; }
  FORCEINLINE void
  setInt(const String& paramName, int value) { m_params[paramName] = {ParamType::kInt, value}; }

 private:

  void
  checkAnimationState(float deltaTime);
  void
  updateParamTriggers();
  void
  transitionTo(const String& name);
  bool
  evaluateTransitionConditions(const AnimationTransition* t) const;

  Map<String, UniquePtr<AnimationNode>>  m_animations;
  AnimationNode*                         m_currentAnimation = nullptr;
  Map<String, Param>                     m_params;
  SpriteComponent*                       m_sprite  = nullptr;
  float                                  m_currentTime = 0.0f;
  AnimationState                         m_state   = AnimationState::kStopped;
};

} // namespace sfmx
