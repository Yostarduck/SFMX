/************************************************************************/
/**
 * @file SourceComponent.h
 * @author Swampertor
 * @date 2026/06/10
 * @brief  Audio source component that wraps sf::Sound / sf::Music.
 */
/************************************************************************/
#pragma once

#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Audio/SoundSource.hpp>

#include "scene/Component.h"
#include "utils/UUID.h"

namespace sfmx
{

class SoundAsset;

/** @brief Audio channel layout hint */
enum class AudioSpace : int32 { kNone = 0, kMono, kStereo, kSurround };

/** @brief Which concrete type backs the m_source pointer */
enum class AudioBackend : int32 { kNone, kSound, kMusic };

/** @brief Current state of the playing audio */
enum class AudioStatus : int32 { kPlaying, kPaused, kStopped };

/**
 * @brief Wraps an sf::Sound or sf::Music behind a single sf::SoundSource*
 *        view pointer, dispatching backend-specific operations by enum.
 */
class SourceComponent : public ComponentT<SourceComponent>
{
public:
  /** @brief Constructs a silent source with no backend loaded */
  SourceComponent(SceneNode* owner);
  ~SourceComponent() override;

  // Loading

  /** @brief Tries loadSoundFromFile first, falls back to loadMusicFromFile */
  bool loadFromFile(const String& filePath);
  /** @brief Loads audio into an internal sf::SoundBuffer + sf::Sound */
  bool loadSoundFromFile(const String& filePath);
  /** @brief Opens a streaming sf::Music from a file path */
  bool loadMusicFromFile(const String& filePath);
  /** @brief Loads from an externally provided SoundBuffer into sf::Sound */
  bool loadFromBuffer(const sf::SoundBuffer& data);

  // Asset-backed loading (the serializable, kSound path)

  /** @brief Bind to a SoundAsset, keeping it alive and recording its UUID; sets up
   *         the kSound backend from the asset's buffer. */
  void setSoundAsset(SPtr<SoundAsset> asset);
  /** @brief Record the sound asset UUID and resolve it via AssetManager when
   *         available; if it can't be resolved the id is still kept. */
  void setSoundAssetId(const UUID& id);
  /** @brief UUID of the referenced SoundAsset, or UUID::null() for music/raw/none. */
  NODISCARD const UUID& getSoundAssetId() const;
  /** @brief The kept-alive SoundAsset, or nullptr if not asset-backed. */
  NODISCARD SPtr<SoundAsset> getSoundAsset() const;

  // Playback

  /** @brief Starts or resumes playback */
  void play();
  /** @brief Pauses playback without rewinding */
  void pause();
  /** @brief Stops and rewinds to the beginning */
  void stop();

  /** @brief Current playback status (Playing / Paused / Stopped) */
  NODISCARD AudioStatus getStatus() const;
  /** @brief Sets the output volume (0-100) */
  void setVolume(float volume);
  /** @brief Returns the current volume */
  NODISCARD float getVolume() const;
  /** @brief Sets the pitch multiplier (1.0 = normal) */
  void setPitch(float pitch);
  /** @brief Returns the current pitch */
  NODISCARD float getPitch() const;
  /** @brief Sets the pan (-1 left, 0 center, 1 right) */
  void setPan(float pan);
  /** @brief Returns the current pan */
  NODISCARD float getPan() const;
  /** @brief Sets whether the source should loop */
  void setLooping(bool loop);
  /** @brief Whether looping is enabled */
  NODISCARD bool isLooping() const;
  /** @brief Seeks to a specific playback offset */
  void setPlayingOffset(sf::Time offset);
  /** @brief Returns the current playback offset */
  NODISCARD sf::Time getPlayingOffset() const;
  /** @brief Total duration of the loaded audio */
  NODISCARD sf::Time getDuration() const;

  // Spatial

  /** @brief Sets the 3D position in the audio scene */
  void setPosition(const sf::Vector3f& position);
  /** @brief Whether position is relative to the listener instead of world */
  void setRelativeToListener(bool relative);
  /** @brief Whether the source uses listener-relative coordinates */
  NODISCARD bool isRelativeToListener() const;
  /** @brief Minimum distance before attenuation kicks in */
  void setMinDistance(float distance);
  /** @brief Gets the minimum distance */
  NODISCARD float getMinDistance() const;
  /** @brief Attenuation factor (how quickly volume drops with distance) */
  void setAttenuation(float attenuation);
  /** @brief Gets the attenuation factor */
  NODISCARD float getAttenuation() const;
  /** @brief Enables or disables 3D spatialization */
  void setSpatializationEnabled(bool enabled);
  /** @brief Whether 3D spatialization is enabled */
  NODISCARD bool isSpatializationEnabled() const;

  // Node-follow

  /** @brief When true, the 3D position is synced from the owner node's world transform each frame */
  void setFollowNode(bool follow);
  /** @brief Whether node-follow is active */
  NODISCARD bool isFollowingNode() const;

  // Component hooks

  /** @brief Drives auto position from the node's world transform when m_followNode is true */
  void onUpdate(float deltaTime) override;

  /** @brief Serializes the source handle (SoundAsset UUID or music path) + playback params. */
  void onSerialize(DataStream& stream) const override;
  /** @brief Restores the source (re-resolves the asset / re-opens the music) and params. */
  void onDeserialize(DataStream& stream) override;

  /** @brief Returns the active SoundSource pointer (may be null) */
  NODISCARD const sf::SoundSource* getSource() const;

private:

  /** @brief Non-const access to the active SoundSource (may be null) */
  NODISCARD sf::SoundSource* activeSource();
  /** @brief Const access to the active SoundSource (may be null) */
  NODISCARD const sf::SoundSource* activeSource() const;

  sf::SoundSource*  m_source = nullptr;
  sf::SoundBuffer   m_buffer;
  sf::Sound         m_sound;
  sf::Music         m_music;
  AudioBackend      m_backend    = AudioBackend::kNone;
  bool              m_followNode = true;

  SPtr<SoundAsset>  m_soundAsset;                 //!< keep-alive for an asset-backed buffer
  UUID              m_soundAssetId = UUID::null();
  String            m_musicPath;                  //!< source path for the streaming backend
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::SourceComponent)
