#include "scripts/RegisterAnimatorComponent.h"

#include "core/platform/Prerequisites.h"
#include "scene/AnimatorComponent.h"

namespace sfmx
{

namespace script
{

void
registerAnimatorComponent(sol::state_view lua) {
  lua.new_usertype<AnimatorComponent>("AnimatorComponent",
    sol::no_constructor,
    sol::base_classes, sol::bases<Component>(),

    "typeId", sol::var(componentTypeId<AnimatorComponent>()),

    "play", sol::overload(
      [](AnimatorComponent& c) {
        c.play();
      },
      [](AnimatorComponent& c, const String& animation) {
        c.play(animation);
      }
    ),
    "pause", &AnimatorComponent::pause,
    "stop", &AnimatorComponent::stop,

    "setCurrentAnimation", &AnimatorComponent::setCurrentAnimation
  );
}

}  // namespace script

}  // namespace sfmx