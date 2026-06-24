#pragma once

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Angle.hpp>

#include "core/platform/Prerequisites.h"
#include "scene/Component.h"
#include "scene/SceneNode.h"
#include "utils/MemoryPoolHandler.h"
#include "utils/TypeTraits.h"
#include "utils/UUID.h"

namespace sf { class VertexBuffer; }

namespace sfmx
{

class DataStream;
class TextureAsset;

/** @brief Controls how active particles are ordered before rendering. */
enum class ParticleSortMode : int32 
{
  kNone,
  kBackToFront
};

/**
 * @brief A single particle: position, velocity, colour, rotation, and lifetime
 *        together with intrusive linked-list hooks for the active set.
 */
struct Particle
{
  sf::Vector2f  position;
  sf::Vector2f  velocity;
  sf::Color     color;
  float         rotation;
  float         angularVelocity;
  float         lifetime;
  float         maxLifetime;
  /** @brief Normalised age, 0 = just spawned, 1 = about to die. */
  float         progress;
  Particle*     prev = nullptr;
  Particle*     next = nullptr;
};

/**
 * @brief Per-emitter configuration that controls how particles are spawned,
 *        how they behave, and how they are rendered.
 */
struct EmitterConfig
{
  size_t              maxParticles    = 256;
  sf::Vector2f        positionOffset  = {0.0f, 0.0f};
  sf::Vector2f        gravity         = {0.f, 0.f};
  sf::Vector2f        startSize       = {8.f, 8.f};
  sf::Vector2f        endSize         = {0.f, 0.f};
  const sf::Texture*  texture         = nullptr;
  //!< Serializable handle to the texture's asset (null for a raw/no texture).
  UUID                textureAssetId  = UUID::null();

  sf::BlendMode       blendMode       = sf::BlendAlpha;

  /** @brief Particles emitted per second (0 = manual @ref emit only). */
  float         emissionRate            = 0.0f;
  float         positionVariance        = 0.0f;
  sf::Angle     direction               = sf::Angle::Zero;
  sf::Angle     directionVariance       = sf::Angle::Zero;
  float         speed                   = 100.0f;
  float         speedVariance           = 0.0f;
  sf::Angle     startRotation           = sf::Angle::Zero;
  sf::Angle     startRotationVariance   = sf::Angle::Zero;
  float         angularVelocity         = 0.f;
  float         angularVelocityVariance = 0.f;
  sf::Color     startColor              = sf::Color::White;
  sf::Color     endColor                = sf::Color(255, 255, 255, 0);
  float         lifetime                = 1.f;
  float         lifetimeVariance        = 0.f;
  float         duration                = 0.f;    // 0 = infinite

  bool          loop                    = false;
};

/**
 * @brief Component that simulates and renders a GPU-efficient particle system.
 *
 * Particles are stored in a shared pool managed by @ref MemoryPoolHandler and
 * linked together in an intrusive doubly-linked list; the vertex buffer is
 * pre-allocated at the maximum capacity so the render path is allocation-free.
 */
class ParticleSystemComponent : public ComponentT<ParticleSystemComponent>
{
 public:
  /** @brief Constructs a particle system on @p owner with default config. */
  explicit ParticleSystemComponent(SceneNode* owner);
  /** @brief Constructs a particle system on @p owner with the given @p config. */
  ParticleSystemComponent(SceneNode* owner, 
                          const EmitterConfig& config);
  /** @brief Destructor, kills all particles still in flight. */
  ~ParticleSystemComponent() override;

  /** @brief Replace the full emitter configuration; particles are cleared. */
  void 
  setConfig(const EmitterConfig& config);
  /** @brief Returns the current emitter configuration. */
  NODISCARD FORCEINLINE const EmitterConfig& 
  getConfig() const { return m_config; }

  /** @brief When true, particle positions are in world space (default: false). */
  FORCEINLINE void 
  setWorldSpace(bool worldSpace) { m_worldSpace = worldSpace; }
  /** @brief Returns the current world-space flag. */
  NODISCARD FORCEINLINE bool 
  isWorldSpace() const { return m_worldSpace; }

  /** @brief Sets how active particles are sorted before rendering by @p mode. */
  FORCEINLINE void 
  setSortMode(ParticleSortMode mode) { m_sortMode = mode; }
  /** @brief Returns the current sort mode. */
  NODISCARD FORCEINLINE ParticleSortMode 
  getSortMode() const { return m_sortMode; }

  /** @brief Sets the particles-per-second rate for auto-emission by @p rate. */
  FORCEINLINE void 
  setEmissionRate(float rate) { m_config.emissionRate = rate; }
  /** @brief Returns the current emission rate. */
  NODISCARD FORCEINLINE float 
  getEmissionRate() const { return m_config.emissionRate; }

  /** @brief Spawn @p count particles immediately from the current config. */
  void emit(size_t count);
  /** @brief Kill all active particles immediately. */
  void clear();
  /** @brief (Re)start the emission timer. */
  void start();
  /** @brief Stop auto-emission; in-flight particles keep simulating. */
  FORCEINLINE void stop() { m_running = false; }

  /** @brief Returns whether auto-emission is running. */
  NODISCARD FORCEINLINE bool  isRunning() const { return m_running; }
  /** @brief Returns the normalised progress of this emitter (0 = just started, 1 = finished). */
  NODISCARD float getProgress() const;

  /** @brief Returns the number of particles currently alive. */
  NODISCARD FORCEINLINE size_t 
  getParticleCount() const { return m_count; }
  /** @brief Returns the maximum number of particles allowed. */
  NODISCARD FORCEINLINE size_t 
  getMaxParticles()  const { return m_capacity; }

  /**
   * @brief Advances the simulation by @p deltaTime seconds.
   *
   * @param deltaTime Seconds elapsed since the previous frame.
   */
  void onUpdate(float deltaTime) override;

  /**
   * @brief Draw hook, called by the owning node's traversal.
   *
   * @param target States the surface to draw onto.
   * @param states Render states carrying the accumulated world transform of
   *               the owning node; pass them straight to @c target.draw.
   */
  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  /** @brief Serializes the emitter config (texture by asset UUID), sort mode and
   *         world-space flag. Live particles and timers are transient and not saved. */
  void onSerialize(DataStream& stream) const override;
  /** @brief Restores the emitter config (re-resolving the texture via AssetManager) and
   *         flags via @ref setConfig; no live particles are restored. */
  void onDeserialize(DataStream& stream) override;

 private:
  /** @brief Allocate a new particle and initialise it from the current config. */
  void spawnParticle();
  /** @brief Remove @p particle from the active list and return it to the pool. */
  void kill(Particle* particle);
  /** @brief Fill the vertex buffer from the active linked list. */
  void rebuildVertices() const;

  /** @brief Current emitter configuration. */
  EmitterConfig       m_config;
  /** @brief Keep-alive for an asset-backed texture; @ref m_config.texture points into it. */
  SPtr<TextureAsset>  m_textureAsset;
  /** @brief Current sort mode for active particles. */
  ParticleSortMode    m_sortMode    = ParticleSortMode::kNone;
  /** @brief If true, particles are emitted in world space (default: local). */
  bool                m_worldSpace  = false;
  /** @brief Accumulated time since the last auto-spawn (fractional emission rates). */
  float               m_accumulator = 0.0f;
  /** @brief Number of particles currently alive. */
  size_t               m_count       = 0;
  /** @brief Maximum number of particles ever allowed. */
  size_t               m_capacity    = 0;

  /** @brief Elapsed time since start() was called (used for duration/loop). */
  float        m_elapsed = 0.f;
  /** @brief Whether auto-emission is active. */
  bool         m_running = true;

  /** @brief Head of the intrusive doubly-linked list of active particles. */
  Particle*    m_firstParticle = nullptr;
  /** @brief Tail of the intrusive doubly-linked list of active particles. */
  Particle*    m_lastParticle  = nullptr;

  /** @brief Pre-allocated vertex buffer (capacity * 6 vertices, two triangles per particle).
   *         Lazily created in setConfig() to avoid OpenGL context dependency at construction. */
  mutable UniquePtr<sf::VertexBuffer> m_vertexBuffer;
  /** @brief Set to true whenever the active list changes, triggers a rebuild before draw. */
  mutable bool m_verticesDirty = true;

};

} // sfmx

DECLARE_TYPE_TRAITS(sfmx::Particle)
DECLARE_TYPE_TRAITS(sfmx::ParticleSystemComponent)