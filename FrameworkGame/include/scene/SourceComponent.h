#pragma once

#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Audio/SoundSource.hpp>

#include "scene/Component.h"

namespace sfmx
{

enum class AudioSpace : uint32
{
  eNONE = 0,
  eMONO,
  eSTEREO,
  eSURROUND
};

class SourceComponent : public ComponentT<SourceComponent>
{
public:
  SourceComponent(SceneNode* owner);
  ~SourceComponent() override;

  // Loading
  bool loadFromFile(const String& filePath);
  bool loadSoundFromFile(const String& filePath);
  bool loadMusicFromFile(const String& filePath);
  bool loadFromBuffer(const sf::SoundBuffer& data);

  // Playback
  void play();
  void pause();
  void stop();

  NODISCARD sf::SoundSource::Status getStatus() const;
  void setVolume(float volume);
  NODISCARD float getVolume() const;
  void setPitch(float pitch);
  NODISCARD float getPitch() const;
  void setPan(float pan);
  NODISCARD float getPan() const;
  void setLooping(bool loop);
  NODISCARD bool isLooping() const;
  void setPlayingOffset(sf::Time offset);
  NODISCARD sf::Time getPlayingOffset() const;
  NODISCARD sf::Time getDuration() const;

  void setPosition(const sf::Vector3f& position);
  void setRelativeToListener(bool relative);
  NODISCARD bool isRelativeToListener() const;
  void setMinDistance(float distance);
  NODISCARD float getMinDistance() const;
  void setAttenuation(float attenuation);
  NODISCARD float getAttenuation() const;
  void setSpatializationEnabled(bool enabled);
  NODISCARD bool isSpatializationEnabled() const;

  // Node-follow: auto-update 3D position from owner's world transform
  void setFollowNode(bool follow);
  NODISCARD bool isFollowingNode() const;

  // Component hooks
  void onUpdate(float deltaTime) override;

private:
  enum class Backend { None, Sound, Music };

  NODISCARD sf::SoundSource* activeSource();
  NODISCARD const sf::SoundSource* activeSource() const;

  sf::SoundSource* m_source = nullptr;
  sf::SoundBuffer  m_buffer;
  sf::Sound        m_sound;
  sf::Music        m_music;
  Backend          m_backend   = Backend::None;
  bool             m_followNode = false;
};

} // namespace sfmx
