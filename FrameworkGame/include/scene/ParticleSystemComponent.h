#pragma once

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/VertexBuffer.hpp>
#include <SFML/System/Angle.hpp>

#include "core/MemoryPool.h"
#include "core/platform/Prerequisites.h"
#include "scene/Component.h"
#include "scene/SceneNode.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

enum class ParticleSortMode
{
  None,
  BackToFront
};

struct Particle
{
  sf::Vector2f  position;
  sf::Vector2f  velocity;
  sf::Color     color;
  float         rotation;
  float         angularVelocity;
  float         lifetime;
  float         maxLifetime;
  float         progress;
};

struct EmitterConfig
{
  float         emissionRate            = 0.0f;
  size_t        maxParticles            = 256;
  sf::Vector2f  positionOffset          = {0.0f, 0.0f};
  float         positionVariance        = 0.0f;
  sf::Angle     direction               = sf::Angle::Zero;
  sf::Angle     directionVariance       = sf::Angle::Zero;
  float         speed                   = 100.0f;
  float         speedVariance           = 0.0f;
  sf::Angle     startRotation           = sf::Angle::Zero;
  sf::Angle     startRotationVariance   = sf::Angle::Zero;
  float         angularVelocity         = 0.f;
  float         angularVelocityVariance = 0.f;

  sf::Vector2f  gravity                 = {0.f, 0.f};

  sf::Color     startColor              = sf::Color::White;
  sf::Color     endColor                = sf::Color(255, 255, 255, 0);

  sf::Vector2f  startSize               = {8.f, 8.f};
  sf::Vector2f  endSize                 = {0.f, 0.f};

  float         lifetime                = 1.f;
  float         lifetimeVariance        = 0.f;

  float         duration                = 0.f;    // 0 = infinite
  bool          loop                    = false;

  const sf::Texture* texture            = nullptr;
  sf::BlendMode      blendMode          = sf::BlendAlpha;
};

class ParticleSystemComponent : public ComponentT<ParticleSystemComponent>
{
 public:
  ParticleSystemComponent(SceneNode* owner);
  ParticleSystemComponent(SceneNode* owner, 
                          const EmitterConfig& config);
  ~ParticleSystemComponent() override;
  
  void setConfig(const EmitterConfig& config);
  NODISCARD const EmitterConfig& getConfig() const;
  
  void setWorldSpace(bool worldSpace);
  NODISCARD bool isWorldSpace() const;
  
  void setSortMode(ParticleSortMode mode);
  NODISCARD ParticleSortMode getSortMode() const;

  void setEmissionRate(float rate);
  NODISCARD float getEmissionRate() const;
  
  void emit(size_t count);
  void clear();
  void start();
  void stop();
  
  NODISCARD bool  isRunning() const { return m_running; }
  NODISCARD float getProgress() const;
  
  NODISCARD size_t getParticleCount() const { return m_count; }
  NODISCARD size_t getMaxParticles() const { return m_capacity; }
  
  void onUpdate(float deltaTime) override;
  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

 private:
  void spawnParticle();
  void kill(size_t index);
  void rebuildVertices() const;
  
  EmitterConfig       m_config;
  ParticleSortMode    m_sortMode    = ParticleSortMode::None;
  bool                m_worldSpace  = false;
  float               m_accumulator = 0.0f;
  MemoryPool<Particle> m_pool;
  Vector<Particle*>    m_active;
  size_t               m_count       = 0;
  size_t               m_capacity    = 0;

  float        m_elapsed = 0.f;
  bool         m_running = true;

  mutable sf::VertexArray  m_scratchVertices;
  mutable sf::VertexBuffer m_vertexBuffer;
  mutable bool m_verticesDirty = true;

};

} // sfmx

DECLARE_TYPE_TRAITS(sfmx::ParticleSystemComponent)