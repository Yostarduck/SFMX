#include "scripts/RegisterParticleSystemComponent.h"

#include <functional>

#include <SFML/Graphics/BlendMode.hpp>

#include "core/platform/Prerequisites.h"
#include "scene/ParticleSystemComponent.h"

namespace sfmx
{

namespace script
{

namespace
{

/**
 * @brief Registers sf::BlendMode together with its Factor/Equation enums and
 *        the common preset modes, so EmitterConfig::blendMode is usable from
 *        Lua (e.g. config.blendMode = BlendMode.Add).
 */
void
registerBlendMode(sol::state_view lua) {
  lua.new_enum<sf::BlendMode::Factor>("BlendFactor", {
    { "Zero",             sf::BlendMode::Factor::Zero },
    { "One",              sf::BlendMode::Factor::One },
    { "SrcColor",         sf::BlendMode::Factor::SrcColor },
    { "OneMinusSrcColor", sf::BlendMode::Factor::OneMinusSrcColor },
    { "DstColor",         sf::BlendMode::Factor::DstColor },
    { "OneMinusDstColor", sf::BlendMode::Factor::OneMinusDstColor },
    { "SrcAlpha",         sf::BlendMode::Factor::SrcAlpha },
    { "OneMinusSrcAlpha", sf::BlendMode::Factor::OneMinusSrcAlpha },
    { "DstAlpha",         sf::BlendMode::Factor::DstAlpha },
    { "OneMinusDstAlpha", sf::BlendMode::Factor::OneMinusDstAlpha }
  });

  lua.new_enum<sf::BlendMode::Equation>("BlendEquation", {
    { "Add",             sf::BlendMode::Equation::Add },
    { "Subtract",        sf::BlendMode::Equation::Subtract },
    { "ReverseSubtract", sf::BlendMode::Equation::ReverseSubtract },
    { "Min",             sf::BlendMode::Equation::Min },
    { "Max",             sf::BlendMode::Equation::Max }
  });

  using Factor = sf::BlendMode::Factor;
  using Equation = sf::BlendMode::Equation;
  lua.new_usertype<sf::BlendMode>("BlendMode",
    sol::call_constructor,
    sol::constructors<
      sf::BlendMode(),
      sf::BlendMode(Factor, Factor),
      sf::BlendMode(Factor, Factor, Equation),
      sf::BlendMode(Factor, Factor, Equation, Factor, Factor, Equation)
    >(),

    "colorSrcFactor", &sf::BlendMode::colorSrcFactor,
    "colorDstFactor", &sf::BlendMode::colorDstFactor,
    "colorEquation",  &sf::BlendMode::colorEquation,
    "alphaSrcFactor", &sf::BlendMode::alphaSrcFactor,
    "alphaDstFactor", &sf::BlendMode::alphaDstFactor,
    "alphaEquation",  &sf::BlendMode::alphaEquation,

    sol::meta_function::equal_to,
    [](const sf::BlendMode& left, const sf::BlendMode& right) {
      return left == right;
    },

    // Common preset modes, mirroring the sf:: globals.
    "Alpha",    sol::var(std::cref(sf::BlendAlpha)),
    "Add",      sol::var(std::cref(sf::BlendAdd)),
    "Multiply", sol::var(std::cref(sf::BlendMultiply)),
    "Min",      sol::var(std::cref(sf::BlendMin)),
    "Max",      sol::var(std::cref(sf::BlendMax)),
    "None",     sol::var(std::cref(sf::BlendNone))
  );
}

}  // namespace

void
registerParticleSystemComponent(sol::state_view lua) {
  registerBlendMode(lua);

  lua.new_enum<ParticleSortMode>("ParticleSortMode", {
    { "None",         ParticleSortMode::kNone },
    { "BackToFront",  ParticleSortMode::kBackToFront }
  });

  lua.new_usertype<EmitterConfig>("EmitterConfig",
    "maxParticles",   &EmitterConfig::maxParticles,
    "positionOffset", &EmitterConfig::positionOffset,
    "gravity",        &EmitterConfig::gravity,
    "startSize",      &EmitterConfig::startSize,
    "endSize",        &EmitterConfig::endSize,
    "texture",        &EmitterConfig::texture, // Not exposed yet
    "blendMode",      &EmitterConfig::blendMode,

    "emissionRate",             &EmitterConfig::emissionRate,
    "positionVariance",         &EmitterConfig::positionVariance,
    "direction",                &EmitterConfig::direction,
    "directionVariance",        &EmitterConfig::directionVariance,
    "speed",                    &EmitterConfig::speed,
    "speedVariance",            &EmitterConfig::speedVariance,
    "startRotation",            &EmitterConfig::startRotation,
    "startRotationVariance",    &EmitterConfig::startRotationVariance,
    "angularVelocity",          &EmitterConfig::angularVelocity,
    "angularVelocityVariance",  &EmitterConfig::angularVelocityVariance,
    "startColor",               &EmitterConfig::startColor,
    "endColor",                 &EmitterConfig::endColor,
    "lifetime",                 &EmitterConfig::lifetime,
    "lifetimeVariance",         &EmitterConfig::lifetimeVariance,
    "duration",                 &EmitterConfig::duration,

    "loop", &EmitterConfig::loop
  );

  lua.new_usertype<ParticleSystemComponent>("ParticleSystemComponent",
    sol::no_constructor,
    sol::base_classes, sol::bases<Component>(),

    "typeId", sol::var(componentTypeId<ParticleSystemComponent>()),

    "setConfig", &ParticleSystemComponent::setConfig, // Not implemented yet
    "getConfig", &ParticleSystemComponent::getConfig, // Not implemented yet

    "setWorldSpace", &ParticleSystemComponent::setWorldSpace,
    "isWorldSpace", &ParticleSystemComponent::isWorldSpace,

    "setSortMode", &ParticleSystemComponent::setSortMode,
    "getSortMode", &ParticleSystemComponent::getSortMode,

    "setEmissionRate", &ParticleSystemComponent::setEmissionRate,
    "getEmissionRate", &ParticleSystemComponent::getEmissionRate,

    "emit", &ParticleSystemComponent::emit,
    "clear", &ParticleSystemComponent::clear,
    "start", &ParticleSystemComponent::start,
    "stop", &ParticleSystemComponent::stop,

    "isRunning", &ParticleSystemComponent::isRunning,
    "getProgress", &ParticleSystemComponent::getProgress,

    "getParticleCount", &ParticleSystemComponent::getParticleCount,
    "getMaxParticles", &ParticleSystemComponent::getMaxParticles
  );
}

}  // namespace script

}  // namespace sfmx