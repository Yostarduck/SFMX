#include "scene/ParticleSystemComponent.h"

#include <algorithm>
#include <cmath>
#include <random>

#include "utils/MemoryPoolHandler.h"

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Transform.hpp>

namespace
{

using namespace sfmx;

float randomRange(float min, float max)
{
  return min + static_cast<float>(std::rand()) / RAND_MAX * (max - min);
}

sf::Color lerpColor(const sf::Color& a,
                    const sf::Color& b,
                    float t)
{
  return sf::Color(
    static_cast<uint8>(static_cast<float>(a.r) +
      (static_cast<float>(b.r) - static_cast<float>(a.r)) * t),
    static_cast<uint8>(static_cast<float>(a.g) +
      (static_cast<float>(b.g) - static_cast<float>(a.g)) * t),
    static_cast<uint8>(static_cast<float>(a.b) +
      (static_cast<float>(b.b) - static_cast<float>(a.b)) * t),
    static_cast<uint8>(static_cast<float>(a.a) +
      (static_cast<float>(b.a) - static_cast<float>(a.a)) * t)
  );
}

sf::Vector2f
lerpSize(const sf::Vector2f& a,
         const sf::Vector2f& b,
         float t)
{
  return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

} // anonymous namespace

namespace sfmx
{

ParticleSystemComponent::ParticleSystemComponent(SceneNode* owner)
  : ComponentT<ParticleSystemComponent>(owner) {}


ParticleSystemComponent::ParticleSystemComponent(SceneNode* owner,
                                                 const EmitterConfig& config)
  : ComponentT<ParticleSystemComponent>(owner),
    m_config(config) {}


ParticleSystemComponent::~ParticleSystemComponent()
{
  clear();
}

void
ParticleSystemComponent::setConfig(const EmitterConfig& config)
{
  m_config = config;
  m_capacity = config.maxParticles;
  m_active.clear();
  m_active.reserve(m_capacity);

  SFMX_ASSERT(MemoryPoolHandler::instance().pool<Particle>().getCapacity() >= m_capacity &&
    "Shared particle pool too small. Call registerPool<Particle>(budget) with a larger budget.");

  // Pre-allocate scratch vertices for the worst case so we never reallocate
  // during gameplay.
  m_scratchVertices.resize(m_capacity * 6);
  m_vertexBuffer.setUsage(sf::VertexBuffer::Usage::Stream);

  m_elapsed = 0.f;
  m_running = true;
  m_verticesDirty = true;
}

const EmitterConfig&
ParticleSystemComponent::getConfig() const
{
  return m_config;
}

void
ParticleSystemComponent::setWorldSpace(bool worldSpace)
{
  m_worldSpace = worldSpace;
}

bool
ParticleSystemComponent::isWorldSpace() const
{
  return m_worldSpace;
}

void
ParticleSystemComponent::setSortMode(ParticleSortMode mode)
{
  m_sortMode = mode;
}

ParticleSortMode
ParticleSystemComponent::getSortMode() const
{
  return m_sortMode;
}

void
ParticleSystemComponent::setEmissionRate(float rate)
{
  m_config.emissionRate = rate;
}

float
ParticleSystemComponent::getEmissionRate() const
{
  return m_config.emissionRate;
}

void
ParticleSystemComponent::emit(size_t count)
{
  const size_t clamped = std::min(count, m_capacity - m_count);
  for (size_t i = 0; i < clamped; ++i)
  {
    spawnParticle();
  }
}

void
ParticleSystemComponent::clear()
{
  auto& pool = MemoryPoolHandler::instance().pool<Particle>();
  for (size_t i = 0; i < m_count; ++i)
    pool.deallocate(m_active[i]);
  m_active.clear();
  m_count = 0;
  m_accumulator = 0.f;
  m_verticesDirty = true;
}

void
ParticleSystemComponent::start()
{
  m_elapsed = 0.f;
  m_running = true;
}

void
ParticleSystemComponent::stop()
{
  m_running = false;
}

float
ParticleSystemComponent::getProgress() const
{
  if (m_config.duration <= 0.f) 
    return 0.f;
  return std::min(m_elapsed / m_config.duration, 1.f);
}

void
ParticleSystemComponent::spawnParticle()
{
  auto& pool = MemoryPoolHandler::instance().pool<Particle>();
  if (m_count >= m_capacity || pool.isFull())
    return;

  const float dirAngle = m_config.direction.asRadians() +
                        randomRange(-m_config.directionVariance.asRadians(),
                                    m_config.directionVariance.asRadians());
  const float spd = m_config.speed +
                    randomRange(-m_config.speedVariance, m_config.speedVariance);
  const float lifetime = m_config.lifetime +
                         randomRange(-m_config.lifetimeVariance,
                                     m_config.lifetimeVariance);
  const float rot = m_config.startRotation.asRadians() +
                    randomRange(-m_config.startRotationVariance.asRadians(),
                                m_config.startRotationVariance.asRadians());
  const float angVel = m_config.angularVelocity +
                       randomRange(-m_config.angularVelocityVariance,
                                   m_config.angularVelocityVariance);

  const float posOffsetX =
      randomRange(-m_config.positionVariance,
                  m_config.positionVariance);
  const float posOffsetY =
      randomRange(-m_config.positionVariance,
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

  if (m_worldSpace)
  {
    const sf::Transform& world = m_owner->getWorldTransform();
    p->position = world.transformPoint(p->position);
    p->velocity = world.transformPoint(p->velocity) -
                  world.transformPoint({0.f, 0.f});
  }

  m_active.push_back(p);
  ++m_count;
  m_verticesDirty = true;
}

void
ParticleSystemComponent::kill(size_t index)
{
  SFMX_ASSERT(index < m_count);
  MemoryPoolHandler::instance().pool<Particle>().deallocate(m_active[index]);

  const size_t last = m_count - 1;
  if (index != last)
  {
    m_active[index] = m_active[last];
  }
  m_active.pop_back();
  --m_count;
  m_verticesDirty = true;
}

void
ParticleSystemComponent::onUpdate(float deltaTime)
{
  if (m_count == 0 && m_config.emissionRate == 0.0f)
  {
    return;
  }

  // Duration / loop
  if (m_config.duration > 0.0f)
  {
    m_elapsed += deltaTime;
    if (m_elapsed >= m_config.duration)
    {
      if (m_config.loop)
      {
        m_elapsed = 0.0f;
        m_running = true;
      }
      else
      {
        m_running = false;
      }
    }
  }

  for (size_t i = 0; i < m_count; ++i)
  {
    Particle* p = m_active[i];
    p->position    += p->velocity * deltaTime;
    p->velocity    += m_config.gravity * deltaTime;
    p->lifetime    -= deltaTime;
    p->rotation    += p->angularVelocity * deltaTime;

    p->progress = p->maxLifetime > 0.f
                  ? 1.f - p->lifetime / p->maxLifetime
                  : 1.f;
    p->color = lerpColor(m_config.startColor, m_config.endColor, p->progress);
  }

  size_t i = 0;
  while (i < m_count)
  {
    if (m_active[i]->lifetime <= 0.f)
    {
      kill(i);
      continue;
    }
    ++i;
  }

  if (m_running && m_config.emissionRate > 0.f)
  {
    m_accumulator += m_config.emissionRate * deltaTime;
    while (m_accumulator >= 1.f && m_count < m_capacity)
    {
      spawnParticle();
      m_accumulator -= 1.f;
    }
  }

  if (m_sortMode == ParticleSortMode::BackToFront && m_count > 1)
  {
    std::sort(m_active.begin(),
              m_active.begin() + static_cast<ptrdiff_t>(m_count),
              [](const Particle* a, const Particle* b)
              {
                  return a->position.y < b->position.y;
              });
  }

  m_verticesDirty = true;
}

void
ParticleSystemComponent::onDraw(sf::RenderTarget& target,
                                sf::RenderStates states) const
{
  if (m_count == 0)
  {
    return;
  }

  rebuildVertices();

  if (m_worldSpace)
  {
    states.transform = sf::Transform::Identity;
  }

  states.blendMode = m_config.blendMode;
  states.texture   = m_config.texture;

  target.draw(m_vertexBuffer, states);
}

void
ParticleSystemComponent::rebuildVertices() const
{
  if (!m_verticesDirty)
  {
    return;
  }

  m_scratchVertices.setPrimitiveType(sf::PrimitiveType::Triangles);
  m_vertexBuffer.setPrimitiveType(sf::PrimitiveType::Triangles);

  const bool hasTexture = m_config.texture != nullptr;
  sf::Vector2f texSize;
  if (hasTexture)
  {
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

  for (size_t i = 0; i < m_count; ++i)
  {
    const Particle& p = *m_active[i];

    const sf::Vector2f halfSize =
      lerpSize(m_config.startSize, m_config.endSize, p.progress) * 0.5f;

    const float cosA = std::cos(p.rotation);
    const float sinA = std::sin(p.rotation);

    const sf::Vector2f localCorners[4] = {
      {-halfSize.x, -halfSize.y},
      { halfSize.x, -halfSize.y},
      { halfSize.x,  halfSize.y},
      {-halfSize.x,  halfSize.y},
    };

    sf::Vector2f worldCorners[4];
    for (int j = 0; j < 4; ++j)
    {
      worldCorners[j] = {
        localCorners[j].x * cosA - localCorners[j].y * sinA + p.position.x,
        localCorners[j].x * sinA + localCorners[j].y * cosA + p.position.y,
      };
    }

    const size_t base = i * 6;
    const sf::Vector2f* uv = hasTexture ? uvs : zeroUV;

    m_scratchVertices[base + 0] = {worldCorners[0], p.color, uv[0]};
    m_scratchVertices[base + 1] = {worldCorners[1], p.color, uv[1]};
    m_scratchVertices[base + 2] = {worldCorners[2], p.color, uv[2]};
    m_scratchVertices[base + 3] = {worldCorners[0], p.color, uv[0]};
    m_scratchVertices[base + 4] = {worldCorners[2], p.color, uv[2]};
    m_scratchVertices[base + 5] = {worldCorners[3], p.color, uv[3]};
  }

  // Upload to GPU buffer
  const size_t vertexCount = m_count * 6;
  if (m_vertexBuffer.getVertexCount() != vertexCount)
  {
    if (!m_vertexBuffer.create(vertexCount))
    {
      return;
    }
  }
  if (!m_vertexBuffer.update(&m_scratchVertices[0], vertexCount, 0))
  {
    return;
  }

  m_verticesDirty = false;
}

} // namespace sfmx
