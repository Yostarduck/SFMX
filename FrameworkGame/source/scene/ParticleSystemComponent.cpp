#include "scene/ParticleSystemComponent.h"

#include <algorithm>
#include <cmath>

#include "utils/MemoryPoolHandler.h"
#include "utils/Random.h"
#include "utils/Arithmetic.h"

#include "assets/AssetManager.h"
#include "assets/TextureAsset.h"
#include "core/DataStream.h"
#include "core/DataStreamTypes.h"

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/VertexBuffer.hpp>
#include <SFML/Graphics/Transform.hpp>

namespace sfmx
{

namespace {
/** @brief ParticleSystemComponent blob layout version; bump on format changes. */
constexpr uint32 kParticleSystemComponentVersion = 1;
} // namespace

ParticleSystemComponent::ParticleSystemComponent(SceneNode* owner)
  : ComponentT<ParticleSystemComponent>(owner) {}


ParticleSystemComponent::ParticleSystemComponent(SceneNode* owner,
                                                 const EmitterConfig& config)
  : ComponentT<ParticleSystemComponent>(owner),
    m_config(config) {}


ParticleSystemComponent::~ParticleSystemComponent() {
  clear();
}

void
ParticleSystemComponent::setConfig(const EmitterConfig& config) {
  m_config = config;
  m_capacity = config.maxParticles;
  m_firstParticle = nullptr;
  m_lastParticle = nullptr;

  SFMX_ASSERT(MemoryPoolHandler::instance().pool<Particle>().getCapacity() >= m_capacity &&
    "Shared particle pool too small. Call registerPool<Particle>(budget) with a larger budget.");

  // Vertex buffer created lazily in rebuildVertices() to avoid OpenGL context
  // dependency during construction / configuration (needed for headless CI).
  m_vertexBuffer.reset();

  m_elapsed = 0.f;
  m_running = true;
  m_verticesDirty = true;
}

void
ParticleSystemComponent::emit(size_t count) {
  const size_t clamped = std::min(count, m_capacity - m_count);
  for (size_t i = 0; i < clamped; ++i) {
    spawnParticle();
  }
}

void
ParticleSystemComponent::clear() {
  if (MemoryPoolHandler::instancePtr()) {
    auto& handler = MemoryPoolHandler::instance();
    if (handler.hasPool<Particle>()) {
      auto& pool = handler.pool<Particle>();
      Particle* p = m_firstParticle;
      while (p) {
        Particle* next = p->next;
        pool.deallocate(p);
        p = next;
      }
    }
  }
  m_firstParticle = nullptr;
  m_lastParticle = nullptr;
  m_count = 0;
  m_accumulator = 0.f;
  m_verticesDirty = true;
}

void
ParticleSystemComponent::start() {
  m_elapsed = 0.f;
  m_running = true;
}

float
ParticleSystemComponent::getProgress() const {
  if (m_config.duration <= 0.f) {
    return 0.f;
  }
  return std::min(m_elapsed / m_config.duration, 1.f);
}

void
ParticleSystemComponent::spawnParticle() {
  auto& pool = MemoryPoolHandler::instance().pool<Particle>();
  if (m_count >= m_capacity || pool.isFull()) {
    return;
  }

  const float dirAngle = m_config.direction.asRadians() +
                         Random::range<float>(-m_config.directionVariance.asRadians(),
                                              m_config.directionVariance.asRadians());
  const float spd = m_config.speed +
                    Random::range<float>(-m_config.speedVariance, m_config.speedVariance);
  const float lifetime = m_config.lifetime +
                         Random::range<float>(-m_config.lifetimeVariance,
                                              m_config.lifetimeVariance);
  const float rot = m_config.startRotation.asRadians() +
                    Random::range<float>(-m_config.startRotationVariance.asRadians(),
                                         m_config.startRotationVariance.asRadians());
  const float angVel = m_config.angularVelocity +
                       Random::range<float>(-m_config.angularVelocityVariance,
                                            m_config.angularVelocityVariance);

  const float posOffsetX =
    Random::range<float>(-m_config.positionVariance,
                         m_config.positionVariance);
  const float posOffsetY =
    Random::range<float>(-m_config.positionVariance,
                         m_config.positionVariance);

  Particle* p = pool.allocate();
  p->position        = m_config.positionOffset +
                       sf::Vector2f(posOffsetX, posOffsetY);
  p->velocity        = sf::Vector2f(std::cos(dirAngle) * spd,
                                    std::sin(dirAngle) * spd);
  p->rotation        = rot;
  p->angularVelocity = angVel;
  p->color           = m_config.startColor;
  p->lifetime        = lifetime;
  p->maxLifetime     = lifetime;
  p->progress        = 0.f;

  if (m_worldSpace) {
    const sf::Transform& world = m_owner->getWorldTransform();
    p->position = world.transformPoint(p->position);
    p->velocity = world.transformPoint(p->velocity) -
                  world.transformPoint({0.f, 0.f});
  }

  // Append to linked list
  p->prev = m_lastParticle;
  p->next = nullptr;
  if (m_lastParticle)
    m_lastParticle->next = p;
  else
    m_firstParticle = p;
  m_lastParticle = p;
  ++m_count;
  m_verticesDirty = true;
}

void
ParticleSystemComponent::kill(Particle* particle) {
  SFMX_ASSERT(particle);

  if (MemoryPoolHandler::instancePtr()) {
    MemoryPoolHandler::instance().pool<Particle>().deallocate(particle);
  }

  // Unlink from doubly-linked list
  if (particle->prev)
    particle->prev->next = particle->next;
  else
    m_firstParticle = particle->next;

  if (particle->next)
    particle->next->prev = particle->prev;
  else
    m_lastParticle = particle->prev;

  --m_count;
  m_verticesDirty = true;
}

void
ParticleSystemComponent::onUpdate(float deltaTime) {
  if (m_count == 0 && m_config.emissionRate == 0.0f) {
    return;
  }

  // Duration / loop
  if (m_config.duration > 0.0f) {
    m_elapsed += deltaTime;
    if (m_elapsed >= m_config.duration) {
      if (m_config.loop) {
        m_elapsed = 0.0f;
        m_running = true;
      }
      else {
        m_running = false;
      }
    }
  }

  for (Particle* p = m_firstParticle; p; p = p->next) {
    p->position    += p->velocity * deltaTime;
    p->velocity    += m_config.gravity * deltaTime;
    p->lifetime    -= deltaTime;
    p->rotation    += p->angularVelocity * deltaTime;

    p->progress = p->maxLifetime > 0.f
                  ? 1.f - p->lifetime / p->maxLifetime
                  : 1.f;
    p->color = lerp::color(m_config.startColor, m_config.endColor, p->progress);
  }

  // Cull expired particles (capture-next pattern)
  {
    Particle* p = m_firstParticle;
    while (p) {
      Particle* next = p->next;
      if (p->lifetime <= 0.f) {
        kill(p);
      }
      p = next;
    }
  }

  // Emit new particles
  if (m_running && m_config.emissionRate > 0.f) {
    m_accumulator += m_config.emissionRate * deltaTime;
    while (m_accumulator >= 1.f && m_count < m_capacity) {
      spawnParticle();
      m_accumulator -= 1.f;
    }
  }

  // Sort BackToFront
  if (m_sortMode == ParticleSortMode::kBackToFront && m_count > 1) {
    Vector<Particle*> sorted;
    sorted.reserve(m_count);
    for (Particle* p = m_firstParticle; p; p = p->next)
      sorted.push_back(p);

    std::sort(sorted.begin(), sorted.end(),
              [](const Particle* a, const Particle* b) {
                  return a->position.y < b->position.y;
              });

    m_firstParticle = sorted.front();
    m_lastParticle  = sorted.back();
    for (size_t i = 0; i < sorted.size(); ++i) {
      sorted[i]->prev = (i > 0) ? sorted[i - 1] : nullptr;
      sorted[i]->next = (i + 1 < sorted.size()) ? sorted[i + 1] : nullptr;
    }
  }

  m_verticesDirty = true;
}

void
ParticleSystemComponent::onDraw(sf::RenderTarget& target,
                                sf::RenderStates states) const {
  if (m_count == 0) {
    return;
  }

  rebuildVertices();

  if (!m_vertexBuffer) {
    return;
  }

  if (m_worldSpace) {
    states.transform = sf::Transform::Identity;
  }

  states.blendMode = m_config.blendMode;
  states.texture   = m_config.texture;

  target.draw(*m_vertexBuffer, 0, m_count * 6, states);
}

void
ParticleSystemComponent::rebuildVertices() const {
  if (!m_verticesDirty) {
    return;
  }

  // Lazy vertex buffer creation — avoids OpenGL context dependency until
  // actual rendering is needed (required for headless CI environments).
  if (!m_vertexBuffer) {
    m_vertexBuffer = MakeUnique<sf::VertexBuffer>();
    if (!m_vertexBuffer->create(m_capacity * 6)) {
      m_vertexBuffer.reset();
      return;
    }
    m_vertexBuffer->setUsage(sf::VertexBuffer::Usage::Stream);
    m_vertexBuffer->setPrimitiveType(sf::PrimitiveType::Triangles);
  }

  // Fixed-size stack buffer for vertex data, uploaded in batches
  static constexpr size_t BATCH_VERTS = 1024;
  sf::Vertex batch[BATCH_VERTS];

  const bool hasTexture = m_config.texture != nullptr;
  sf::Vector2f texSize;
  if (hasTexture) {
    const sf::Vector2u ts = m_config.texture->getSize();
    texSize = sf::Vector2f(static_cast<float>(ts.x),
                            static_cast<float>(ts.y));
  }

  const sf::Vector2f uvs[4] = {
    {0.f,        0.f},
    {texSize.x,  0.f},
    {texSize.x,  texSize.y},
    {0.f,        texSize.y},
  };
  const sf::Vector2f zeroUV[4] = {{}, {}, {}, {}};

  size_t batchFilled = 0;
  size_t uploadOffset = 0;

  for (Particle* p = m_firstParticle; p; p = p->next) {
    const sf::Vector2f halfSize =
      lerp::vector2(m_config.startSize, m_config.endSize, p->progress) * 0.5f;

    const float cosA = std::cos(p->rotation);
    const float sinA = std::sin(p->rotation);

    const sf::Vector2f localCorners[4] = {
      {-halfSize.x, -halfSize.y},
      { halfSize.x, -halfSize.y},
      { halfSize.x,  halfSize.y},
      {-halfSize.x,  halfSize.y},
    };

    sf::Vector2f worldCorners[4];
    for (int j = 0; j < 4; ++j) {
      worldCorners[j] = {
        localCorners[j].x * cosA - localCorners[j].y * sinA + p->position.x,
        localCorners[j].x * sinA + localCorners[j].y * cosA + p->position.y,
      };
    }

    const sf::Vector2f* uv = hasTexture ? uvs : zeroUV;

    batch[batchFilled + 0] = {worldCorners[0], p->color, uv[0]};
    batch[batchFilled + 1] = {worldCorners[1], p->color, uv[1]};
    batch[batchFilled + 2] = {worldCorners[2], p->color, uv[2]};
    batch[batchFilled + 3] = {worldCorners[0], p->color, uv[0]};
    batch[batchFilled + 4] = {worldCorners[2], p->color, uv[2]};
    batch[batchFilled + 5] = {worldCorners[3], p->color, uv[3]};
    batchFilled += 6;

    if (batchFilled + 6 > BATCH_VERTS || p == m_lastParticle) {
      if (!m_vertexBuffer->update(batch, batchFilled, uploadOffset))
        return;
      uploadOffset += batchFilled;
      batchFilled = 0;
    }
  }

  m_verticesDirty = false;
}

// -----------------------------------------------------------------------------
// Serialization
// -----------------------------------------------------------------------------

void
ParticleSystemComponent::onSerialize(DataStream& stream) const {
  stream << kParticleSystemComponentVersion;

  stream << static_cast<uint64>(m_config.maxParticles);
  stream << m_config.positionOffset.x << m_config.positionOffset.y;
  stream << m_config.gravity.x << m_config.gravity.y;
  stream << m_config.startSize.x << m_config.startSize.y;
  stream << m_config.endSize.x << m_config.endSize.y;

  stream << m_config.textureAssetId;

  stream << static_cast<uint8>(m_config.blendMode.colorSrcFactor);
  stream << static_cast<uint8>(m_config.blendMode.colorDstFactor);
  stream << static_cast<uint8>(m_config.blendMode.colorEquation);
  stream << static_cast<uint8>(m_config.blendMode.alphaSrcFactor);
  stream << static_cast<uint8>(m_config.blendMode.alphaDstFactor);
  stream << static_cast<uint8>(m_config.blendMode.alphaEquation);

  stream << m_config.emissionRate;
  stream << m_config.positionVariance;
  stream << m_config.direction.asRadians();
  stream << m_config.directionVariance.asRadians();
  stream << m_config.speed;
  stream << m_config.speedVariance;
  stream << m_config.startRotation.asRadians();
  stream << m_config.startRotationVariance.asRadians();
  stream << m_config.angularVelocity;
  stream << m_config.angularVelocityVariance;

  stream << m_config.startColor.r << m_config.startColor.g
         << m_config.startColor.b << m_config.startColor.a;
  stream << m_config.endColor.r << m_config.endColor.g
         << m_config.endColor.b << m_config.endColor.a;

  stream << m_config.lifetime;
  stream << m_config.lifetimeVariance;
  stream << m_config.duration;
  stream << static_cast<uint8>(m_config.loop ? 1 : 0);

  stream << static_cast<int32>(m_sortMode);
  stream << static_cast<uint8>(m_worldSpace ? 1 : 0);
}

void
ParticleSystemComponent::onDeserialize(DataStream& stream) {
  uint32 version = 0;
  stream >> version;
  if (version != kParticleSystemComponentVersion) {
    return;
  }

  EmitterConfig cfg;

  uint64 maxParticles = 0;
  stream >> maxParticles;
  cfg.maxParticles = static_cast<size_t>(maxParticles);
  stream >> cfg.positionOffset.x >> cfg.positionOffset.y;
  stream >> cfg.gravity.x >> cfg.gravity.y;
  stream >> cfg.startSize.x >> cfg.startSize.y;
  stream >> cfg.endSize.x >> cfg.endSize.y;

  stream >> cfg.textureAssetId;

  uint8 colorSrc = 0, colorDst = 0, colorEq = 0;
  uint8 alphaSrc = 0, alphaDst = 0, alphaEq = 0;
  stream >> colorSrc >> colorDst >> colorEq >> alphaSrc >> alphaDst >> alphaEq;
  cfg.blendMode.colorSrcFactor = static_cast<sf::BlendMode::Factor>(colorSrc);
  cfg.blendMode.colorDstFactor = static_cast<sf::BlendMode::Factor>(colorDst);
  cfg.blendMode.colorEquation  = static_cast<sf::BlendMode::Equation>(colorEq);
  cfg.blendMode.alphaSrcFactor = static_cast<sf::BlendMode::Factor>(alphaSrc);
  cfg.blendMode.alphaDstFactor = static_cast<sf::BlendMode::Factor>(alphaDst);
  cfg.blendMode.alphaEquation  = static_cast<sf::BlendMode::Equation>(alphaEq);

  stream >> cfg.emissionRate;
  stream >> cfg.positionVariance;
  float direction = 0.f, directionVar = 0.f;
  stream >> direction >> directionVar;
  cfg.direction         = sf::radians(direction);
  cfg.directionVariance = sf::radians(directionVar);
  stream >> cfg.speed;
  stream >> cfg.speedVariance;
  float startRot = 0.f, startRotVar = 0.f;
  stream >> startRot >> startRotVar;
  cfg.startRotation         = sf::radians(startRot);
  cfg.startRotationVariance = sf::radians(startRotVar);
  stream >> cfg.angularVelocity;
  stream >> cfg.angularVelocityVariance;

  stream >> cfg.startColor.r >> cfg.startColor.g >> cfg.startColor.b >> cfg.startColor.a;
  stream >> cfg.endColor.r >> cfg.endColor.g >> cfg.endColor.b >> cfg.endColor.a;

  stream >> cfg.lifetime;
  stream >> cfg.lifetimeVariance;
  stream >> cfg.duration;
  uint8 loop = 0;
  stream >> loop;
  cfg.loop = (loop != 0);

  int32 sortMode = 0;
  stream >> sortMode;
  uint8 worldSpace = 0;
  stream >> worldSpace;

  // Re-resolve the texture by asset UUID (kept alive so cfg.texture stays valid).
  if (cfg.textureAssetId != UUID::null() && AssetManager::isStarted()) {
    SPtr<TextureAsset> asset =
        AssetManager::instance().load<TextureAsset>(cfg.textureAssetId);
    if (nullptr != asset && asset->isLoaded()) {
      m_textureAsset = asset;
      cfg.texture    = &asset->texture();
    }
  }

  setConfig(cfg);
  setSortMode(static_cast<ParticleSortMode>(sortMode));
  setWorldSpace(worldSpace != 0);
}

} // namespace sfmx
