#include "scene/SourceComponent.h"

#include <SFML/System/Vector3.hpp>

#include "scene/SceneNode.h"
#include "scene/Transform.h"

#include "assets/AssetManager.h"
#include "assets/SoundAsset.h"
#include "core/DataStream.h"
#include "core/DataStreamTypes.h"   // operator<< / >> for UUID
#include "core/FileSystem.h"        // resolve() against the content root

namespace sfmx
{

namespace {
/** @brief SourceComponent blob layout version; bump on format changes. */
constexpr uint32 kSourceComponentVersion = 1;
} // namespace

SourceComponent::SourceComponent(SceneNode* owner)
  : ComponentT<SourceComponent>(owner),
    m_sound(m_buffer)
{
}

SourceComponent::~SourceComponent() {
  // Stop any active audio before the SFML sound/music objects are destroyed.
  stop();
  m_source = nullptr;
  m_backend = AudioBackend::kNone;
}

// -----------------------------------------------------------------------------
// Loading
// -----------------------------------------------------------------------------

bool
SourceComponent::loadFromFile(const String& filePath) {
  // Try Sound first; if the buffer load fails, fall back to Music.
  if (loadSoundFromFile(filePath))
    return true;
  return loadMusicFromFile(filePath);
}

bool
SourceComponent::loadSoundFromFile(const String& filePath) {
  stop();
  m_backend = AudioBackend::kNone;
  m_source = nullptr;

  if (!m_buffer.loadFromFile(FileSystem::resolve(filePath)))
    return false;

  m_sound.setBuffer(m_buffer);
  m_source = &m_sound;
  m_backend = AudioBackend::kSound;
  m_musicPath.clear();  // a raw file-loaded buffer is not the streaming backend
  return true;
}

bool
SourceComponent::loadMusicFromFile(const String& filePath) {
  stop();
  m_backend = AudioBackend::kNone;
  m_source = nullptr;

  // Resolve to open, but remember the ORIGINAL (relative) path so the serialized
  // scene stays portable and re-opens against the content root on load.
  if (!m_music.openFromFile(FileSystem::resolve(filePath))) {
    return false;
  }

  m_source = &m_music;
  m_backend = AudioBackend::kMusic;
  m_musicPath = filePath;        // remembered so the streaming source re-opens on load
  m_soundAssetId = UUID::null(); // music is path-backed, not asset-backed
  return true;
}

bool
SourceComponent::loadFromBuffer(const sf::SoundBuffer& data) {
  stop();
  m_backend = AudioBackend::kNone;
  m_source = nullptr;

  m_buffer = data;
  m_sound.setBuffer(m_buffer);
  m_source = &m_sound;
  m_backend = AudioBackend::kSound;
  m_musicPath.clear();
  return true;
}

// -----------------------------------------------------------------------------
// Asset-backed loading
// -----------------------------------------------------------------------------

void
SourceComponent::setSoundAsset(SPtr<SoundAsset> asset) {
  // If handed an asset that isn't decoded yet, bring it up through the
  // AssetManager by its UUID. A single attempt (no recursion) — same as sprite.
  if (nullptr != asset && !asset->isLoaded() && AssetManager::isStarted()) {
    SPtr<SoundAsset> loaded =
        AssetManager::instance().load<SoundAsset>(asset->metadata().uuid);
    if (nullptr != loaded) {
      asset = loaded;
    }
  }

  m_soundAsset   = asset;
  m_soundAssetId = (nullptr != asset) ? asset->metadata().uuid : UUID::null();
  if (nullptr != asset && asset->isLoaded()) {
    loadFromBuffer(asset->buffer());  // copies into m_buffer, sets up the kSound backend
  }
}

void
SourceComponent::setSoundAssetId(const UUID& id) {
  if (id != UUID::null() && AssetManager::isStarted()) {
    SPtr<SoundAsset> asset = AssetManager::instance().load<SoundAsset>(id);
    if (nullptr != asset) {
      setSoundAsset(asset);  // records m_soundAssetId from the asset (== id)
      return;
    }
  }
  // Couldn't resolve: keep the id so it still re-serializes and can resolve later.
  m_soundAssetId = id;
}

const UUID&
SourceComponent::getSoundAssetId() const {
  return m_soundAssetId;
}

SPtr<SoundAsset>
SourceComponent::getSoundAsset() const {
  return m_soundAsset;
}

// -----------------------------------------------------------------------------
// Playback
// -----------------------------------------------------------------------------

void
SourceComponent::play() {
  if (m_source)
    m_source->play();
}

void
SourceComponent::pause() {
  if (m_source)
    m_source->pause();
}

void
SourceComponent::stop() {
  if (m_source)
    m_source->stop();
}

AudioStatus
SourceComponent::getStatus() const {
  if (nullptr == m_source) {
    return AudioStatus::kStopped;
  }
  
  sf::SoundSource::Status status = m_source->getStatus();
  
  switch (status)
  {
  case sf::SoundSource::Status::Playing:  return AudioStatus::kPlaying;
  case sf::SoundSource::Status::Paused:   return AudioStatus::kPaused;
  case sf::SoundSource::Status::Stopped:  return AudioStatus::kStopped;
  default: break;
  }

  return AudioStatus::kStopped;
}

// -----------------------------------------------------------------------------
// SoundSource passthrough
// -----------------------------------------------------------------------------

void
SourceComponent::setVolume(float volume) {
  if (m_source)
    m_source->setVolume(volume);
}

float
SourceComponent::getVolume() const {
  return m_source ? m_source->getVolume() : 100.f;
}

void
SourceComponent::setPitch(float pitch) {
  if (m_source)
    m_source->setPitch(pitch);
}

float
SourceComponent::getPitch() const {
  return m_source ? m_source->getPitch() : 1.f;
}

void
SourceComponent::setPan(float pan) {
  if (m_source)
    m_source->setPan(pan);
}

float
SourceComponent::getPan() const {
  return m_source ? m_source->getPan() : 0.f;
}

void
SourceComponent::setLooping(bool loop) {
  switch (m_backend) {
    case AudioBackend::kSound: m_sound.setLooping(loop); break;
    case AudioBackend::kMusic: m_music.setLooping(loop); break;
    default: break;
  }
}

bool
SourceComponent::isLooping() const {
  switch (m_backend) {
    case AudioBackend::kSound: return m_sound.isLooping();
    case AudioBackend::kMusic: return m_music.isLooping();
    default:             return false;
  }
}

void
SourceComponent::setPlayingOffset(sf::Time offset) {
  switch (m_backend) {
    case AudioBackend::kSound: m_sound.setPlayingOffset(offset); break;
    case AudioBackend::kMusic: m_music.setPlayingOffset(offset); break;
    default: break;
  }
}

sf::Time
SourceComponent::getPlayingOffset() const {
  switch (m_backend) {
    case AudioBackend::kSound: return m_sound.getPlayingOffset();
    case AudioBackend::kMusic: return m_music.getPlayingOffset();
    default:             return sf::Time::Zero;
  }
}

sf::Time
SourceComponent::getDuration() const {
  switch (m_backend) {
    case AudioBackend::kSound: return m_buffer.getDuration();
    case AudioBackend::kMusic: return m_music.getDuration();
    default:             return sf::Time::Zero;
  }
}

// -----------------------------------------------------------------------------
// Spatial
// -----------------------------------------------------------------------------

void
SourceComponent::setPosition(const sf::Vector3f& position) {
  if (m_source)
    m_source->setPosition(position);
}

void
SourceComponent::setRelativeToListener(bool relative) {
  if (m_source)
    m_source->setRelativeToListener(relative);
}

bool
SourceComponent::isRelativeToListener() const {
  return m_source && m_source->isRelativeToListener();
}

void
SourceComponent::setMinDistance(float distance) {
  if (m_source)
    m_source->setMinDistance(distance);
}

float
SourceComponent::getMinDistance() const {
  return m_source ? m_source->getMinDistance() : 1.f;
}

void
SourceComponent::setAttenuation(float attenuation) {
  if (m_source)
    m_source->setAttenuation(attenuation);
}

float
SourceComponent::getAttenuation() const {
  return m_source ? m_source->getAttenuation() : 1.f;
}

void
SourceComponent::setSpatializationEnabled(bool enabled) {
  if (m_source)
    m_source->setSpatializationEnabled(enabled);
}

bool
SourceComponent::isSpatializationEnabled() const {
  return m_source && m_source->isSpatializationEnabled();
}

// -----------------------------------------------------------------------------
// Node follow
// -----------------------------------------------------------------------------

void
SourceComponent::setFollowNode(bool follow) {
  m_followNode = follow;
}

bool
SourceComponent::isFollowingNode() const {
  return m_followNode;
}

// -----------------------------------------------------------------------------
// Component hook
// -----------------------------------------------------------------------------

void
SourceComponent::onUpdate(float deltaTime) {
  SFMX_PARAMETER_UNUSED(deltaTime);
  if (!m_followNode || !m_source)
    return;

  const sf::Vector2f worldPos =
      m_owner->transform().getWorldTransform().transformPoint({0,0});
  m_source->setPosition({worldPos.x, worldPos.y, 0.f});
  // std::cout << m_source->getPosition().x << " : " << m_source->getPosition().y << std::endl;
}

// -----------------------------------------------------------------------------
// Serialization
// -----------------------------------------------------------------------------

void
SourceComponent::onSerialize(DataStream& stream) const {
  stream << kSourceComponentVersion;
  stream << static_cast<uint8>(m_backend);

  switch (m_backend) {
    case AudioBackend::kSound: stream << m_soundAssetId;     break;
    case AudioBackend::kMusic: stream.writeString(m_musicPath); break;
    default: break;  // kNone: no source handle
  }

  // Playback params (the getters return safe defaults when m_source is null).
  stream << getVolume() << getPitch() << getPan();
  stream << static_cast<uint8>(isLooping() ? 1 : 0);
  stream << static_cast<uint8>(isRelativeToListener() ? 1 : 0);
  stream << getMinDistance() << getAttenuation();
  stream << static_cast<uint8>(isSpatializationEnabled() ? 1 : 0);
  stream << static_cast<uint8>(m_followNode ? 1 : 0);
}

void
SourceComponent::onDeserialize(DataStream& stream) {
  uint32 version = 0;
  stream >> version;
  if (version != kSourceComponentVersion) {
    return;  // unknown version: leave defaults rather than misread bytes
  }

  uint8 backend = 0;
  stream >> backend;
  switch (static_cast<AudioBackend>(backend)) {
    case AudioBackend::kSound: {
      UUID id;
      stream >> id;
      setSoundAssetId(id);  // re-resolves the asset → sets up the kSound backend
      break;
    }
    case AudioBackend::kMusic: {
      m_musicPath = stream.readString();
      if (!m_musicPath.empty()) {
        loadMusicFromFile(m_musicPath);  // re-opens the streaming source
      }
      break;
    }
    default:
      break;  // kNone: no source to rebuild
  }

  // Always consume the param bytes, even if the source couldn't be rebuilt.
  float volume = 100.f;
  float pitch = 1.f;
  float pan = 0.f;
  uint8 looping = 0;
  uint8 relative = 0;
  float minDistance = 1.f;
  float attenuation = 1.f;
  uint8 spatialization = 1;
  uint8 followNode = 1;
  stream >> volume >> pitch >> pan >> looping >> relative
         >> minDistance >> attenuation >> spatialization >> followNode;

  if (nullptr != m_source) {
    setVolume(volume);
    setPitch(pitch);
    setPan(pan);
    setLooping(looping != 0);
    setRelativeToListener(relative != 0);
    setMinDistance(minDistance);
    setAttenuation(attenuation);
    setSpatializationEnabled(spatialization != 0);
  }
  setFollowNode(followNode != 0);
}

// -----------------------------------------------------------------------------
// Internal helpers
// -----------------------------------------------------------------------------

sf::SoundSource*
SourceComponent::activeSource() {
  return m_source;
}

const sf::SoundSource*
SourceComponent::activeSource() const {
  return m_source;
}

const sf::SoundSource*
SourceComponent::getSource() const {
  return m_source;
}

} // namespace sfmx
