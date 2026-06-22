#include "scripts/RegisterSourceComponent.h"

#include "core/platform/Prerequisites.h"
#include "scene/SourceComponent.h"

namespace sfmx
{

namespace script
{

void
registerSourceComponent(sol::state_view lua) {
  lua.new_enum<AudioSpace>("AudioSpace", {
    { "None",     AudioSpace::kNone },
    { "Mono",     AudioSpace::kMono },
    { "Stereo",   AudioSpace::kStereo },
    { "Surround", AudioSpace::kSurround }
  });

  lua.new_enum<AudioBackend>("AudioBackend", {
    { "None",   AudioBackend::kNone },
    { "Sound",  AudioBackend::kSound },
    { "Music",  AudioBackend::kMusic }
  });

  lua.new_enum<AudioStatus>("AudioStatus", {
    { "Playing",  AudioStatus::kPlaying },
    { "Paused",   AudioStatus::kPaused },
    { "Stopped",  AudioStatus::kStopped }
  });

  lua.new_usertype<SourceComponent>("SourceComponent",
    sol::no_constructor,
    sol::base_classes, sol::bases<Component>(),

    "typeId", sol::var(componentTypeId<SourceComponent>()),

    "loadFromFile", &SourceComponent::loadFromFile,
    "loadSoundFromFile", &SourceComponent::loadSoundFromFile,
    "loadMusicFromFile", &SourceComponent::loadMusicFromFile,

    "play", &SourceComponent::play,
    "pause", &SourceComponent::pause,
    "stop", &SourceComponent::stop,

    "getStatus", &SourceComponent::getStatus,
    "setVolume", &SourceComponent::setVolume,
    "getVolume", &SourceComponent::getVolume,
    "setPitch", &SourceComponent::setPitch,
    "getPitch", &SourceComponent::getPitch,
    "setPan", &SourceComponent::setPan,
    "getPan", &SourceComponent::getPan,
    "setLooping", &SourceComponent::setLooping,
    "isLooping", &SourceComponent::isLooping,
    "setPlayingOffset", [](SourceComponent& t, float seconds) {
      t.setPlayingOffset(sf::seconds(seconds));
    },
    "getPlayingOffset", [](SourceComponent& t) {
      return t.getPlayingOffset().asSeconds();
    },
    "getDuration", [](SourceComponent& t) {
      return t.getDuration().asSeconds();
    },

    "setPosition", &SourceComponent::setPosition,
    "setRelativeToListener", &SourceComponent::setRelativeToListener,
    "isRelativeToListener", &SourceComponent::isRelativeToListener,
    "setMinDistance", &SourceComponent::setMinDistance,
    "getMinDistance", &SourceComponent::getMinDistance,
    "setAttenuation", &SourceComponent::setAttenuation,
    "getAttenuation", &SourceComponent::getAttenuation,
    "setSpatializationEnabled", &SourceComponent::setSpatializationEnabled,
    "isSpatializationEnabled", &SourceComponent::isSpatializationEnabled,

    "setFollowNode", &SourceComponent::setFollowNode,
    "isFollowingNode", &SourceComponent::isFollowingNode
  );
}

}  // namespace script

}  // namespace sfmx