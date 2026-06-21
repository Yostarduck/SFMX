#include "scene/SourceComponent.h"

#include <SFML/System/Vector3.hpp>

#include "scene/SceneNode.h"
#include "scene/Transform.h"

namespace sfmx
{

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

  if (!m_buffer.loadFromFile(filePath))
    return false;

  m_sound.setBuffer(m_buffer);
  m_source = &m_sound;
  m_backend = AudioBackend::kSound;
  return true;
}

bool
SourceComponent::loadMusicFromFile(const String& filePath) {
  stop();
  m_backend = AudioBackend::kNone;
  m_source = nullptr;

  if (!m_music.openFromFile(filePath))
    return false;

  m_source = &m_music;
  m_backend = AudioBackend::kMusic;
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
  return true;
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
  default: return AudioStatus::kStopped;
  }
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
