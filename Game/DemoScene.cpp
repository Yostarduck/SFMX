#include "DemoScene.h"

#include <SFML/Graphics.hpp>

#include "scene/Scene.h"
#include "scene/CameraComponent.h"
#include "scene/SourceComponent.h"
#include "scene/ListenerComponent.h"
#include "scene/SpriteComponent.h"
#include "scene/AnimatorComponent.h"
#include "scene/ColliderComponent.h"
#include "scene/RigidBodyComponent.h"
#include "scene/ParticleSystemComponent.h"
#include "scene/ScriptComponent.h"
#include "scene/ComponentRegistry.h"
#include "scene/CanvasComponent.h"
#include "ui/UIButton.h"
#include "ui/UILabel.h"
#include "ui/UIImage.h"
#include "ui/UICheckbox.h"
#include "ui/UITextBox.h"
#include "ui/UISlider.h"

#include "resource/SpriteAtlas.h"
#include "resource/Frame.h"

#include "utils/MemoryPoolHandler.h"

#include "assets/AssetManager.h"
#include "assets/TextureAsset.h"

#include "core/DataStream.h"
#include "core/DataStreamTypes.h"

#include <iostream>

using namespace sfmx;

// Debug-viz component: a filled circle drawn at the node. Serializable (radius +
// color) so it round-trips through the cooker. Created at level-build and
// long-lived, so its sf::CircleShape allocates once at construction (setup, not
// per-frame churn) — acceptable for a build-time object.
class CircleComponent : public ComponentT<CircleComponent>
{
 public:
  explicit CircleComponent(SceneNode* owner)
    : ComponentT<CircleComponent>(owner) {}

  CircleComponent(SceneNode* owner, float radius, sf::Color color)
    : ComponentT<CircleComponent>(owner) {
    setCircle(radius, color);
  }

  void
  setCircle(float radius, sf::Color color) {
    m_circle.setRadius(radius);
    m_circle.setFillColor(color);
    m_circle.setOrigin({radius, radius});
  }

  void
  onDraw(sf::RenderTarget& target, sf::RenderStates states) const override {
    target.draw(m_circle, states);
  }

  void
  onSerialize(DataStream& stream) const override {
    stream << static_cast<uint32>(1);  // version
    stream << m_circle.getRadius();
    const sf::Color color = m_circle.getFillColor();
    stream << color.r << color.g << color.b << color.a;
  }

  void
  onDeserialize(DataStream& stream) override {
    uint32 version = 0;
    stream >> version;
    if (1u != version) {
      return;
    }
    float radius = 0.f;
    stream >> radius;
    uint8 r = 0, g = 0, b = 0, a = 0;
    stream >> r >> g >> b >> a;
    setCircle(radius, sf::Color(r, g, b, a));
  }

 private:
  sf::CircleShape m_circle;
};

DECLARE_TYPE_TRAITS(CircleComponent)

namespace demo {

namespace {

// Resolve a cooked texture by its source-relative name (the id the cooker used).
SPtr<TextureAsset>
loadTex(const ansichar* rel) {
  SPtr<TextureAsset> asset =
      AssetManager::instance().load<TextureAsset>(sfmx::UUID::createFromName(rel));
  if (nullptr == asset) {
    std::cerr << "[Assets] missing: " << rel << " (a build runs --cook)\n";
  }
  return asset;
}

SceneNode*
firstByName(Scene& scene, StringView name) {
  Vector<SceneNode*> nodes = scene.findNodesByName(name);
  return nodes.empty() ? nullptr : nodes.front();
}

} // namespace

void
registerDemoPools(MemoryPoolHandler& pools) {
  pools.registerPool<SceneNode>(1024);
  pools.registerPool<CircleComponent>(64);
  pools.registerPool<SourceComponent>(4);
  pools.registerPool<ListenerComponent>(1);
  pools.registerPool<CameraComponent>(1);
  pools.registerPool<SpriteComponent>(8);
  pools.registerPool<AnimatorComponent>(8);
  pools.registerPool<Particle>(2048);
  pools.registerPool<ParticleSystemComponent>(8);
  pools.registerPool<ColliderComponent>(64);
  pools.registerPool<RigidBodyComponent>(64);
  pools.registerPool<ScriptComponent>(1024);
  pools.registerPool<UIButton>(64);
  pools.registerPool<UILabel>(64);
  pools.registerPool<UIImage>(64);
  pools.registerPool<UICheckbox>(64);
  pools.registerPool<UITextBox>(64);
  pools.registerPool<UISlider>(64);
  pools.registerPool<CanvasComponent>(8);
}

void
registerDemoComponents() {
  ComponentRegistry& reg = ComponentRegistry::instance();
  reg.registerComponent<CircleComponent>();
  reg.registerComponent<SourceComponent>();
  reg.registerComponent<ListenerComponent>();
  reg.registerComponent<CameraComponent>();
  reg.registerComponent<SpriteComponent>();
  reg.registerComponent<AnimatorComponent>();
  reg.registerComponent<ParticleSystemComponent>();
  reg.registerComponent<ColliderComponent>();
  reg.registerComponent<RigidBodyComponent>();
  reg.registerComponent<ScriptComponent>();
  reg.registerComponent<CanvasComponent>();
  reg.registerComponent<UIButton>();
  reg.registerComponent<UILabel>();
  reg.registerComponent<UIImage>();
  reg.registerComponent<UICheckbox>();
  reg.registerComponent<UITextBox>();
  reg.registerComponent<UISlider>();
}

void
buildDemoScene(Scene& scene, float windowWidth, float windowHeight) {
  const sf::Vector2f center = {windowWidth * 0.5f, windowHeight * 0.5f};

  SceneNode* sun = scene.createNode("Sun");
  sun->transform().setPosition(center);
  sun->addComponent<CircleComponent>(40.f, sf::Color(255, 180, 100));

  SPtr<TextureAsset> particleAsset = loadTex("particle.png");
  const sf::Texture* particleTex =
      (nullptr != particleAsset) ? &particleAsset->texture() : nullptr;
  const sfmx::UUID particleId =
      (nullptr != particleAsset) ? particleAsset->metadata().uuid : sfmx::UUID::null();

  // -- Sun: fiery corona (local space, follows the sun's rotation) --
  EmitterConfig sunCfg;
  sunCfg.emissionRate          = 40.f;
  sunCfg.maxParticles          = 150;
  sunCfg.direction             = sf::degrees(0.f);
  sunCfg.directionVariance     = sf::degrees(360.f);
  sunCfg.speed                 = 120.f;
  sunCfg.speedVariance         = 40.f;
  sunCfg.startRotation         = sf::degrees(0.f);
  sunCfg.startRotationVariance = sf::degrees(360.f);
  sunCfg.angularVelocity       = 90.f;
  sunCfg.angularVelocityVariance = 45.f;
  sunCfg.gravity               = {-40.f, 0.f};
  sunCfg.startColor            = sf::Color(255, 200, 80);
  sunCfg.endColor              = sf::Color(255, 50, 0, 0);
  sunCfg.startSize             = {20.f, 20.f};
  sunCfg.endSize               = {0.f, 0.f};
  sunCfg.lifetime              = 5.0f;
  sunCfg.lifetimeVariance      = 0.5f;
  sunCfg.texture               = particleTex;
  sunCfg.textureAssetId        = particleId;
  sunCfg.blendMode             = sf::BlendAlpha;
  sunCfg.duration              = 0.f;
  sunCfg.loop                  = false;

  auto* sunParticles = sun->addComponent<ParticleSystemComponent>();
  sunParticles->setConfig(sunCfg);
  sunParticles->setSortMode(ParticleSortMode::kBackToFront);
  sunParticles->setWorldSpace(false);

  SceneNode* sun2 = scene.createNode("Sun2");
  sun2->transform().setPosition(center);

  SceneNode* cameraNode = scene.createNode("Camera", sun);
  auto* camera = cameraNode->addComponent<CameraComponent>();
  camera->setSize({windowWidth * 2.f, windowHeight * 2.f});
  camera->setFollowNode(true);

  SceneNode* earth = scene.createNode("Earth", sun);
  earth->transform().setPosition({140.f, 0.f});
  earth->addComponent<CircleComponent>(20.f, sf::Color(100, 180, 255));
  auto* bgm = earth->addComponent<SourceComponent>();
  if (bgm->loadMusicFromFile("resources/background.mp3")) {
    bgm->setLooping(true);
    bgm->setVolume(1.0f);
    bgm->setSpatializationEnabled(false);
  }
  else {
    std::cout << "[Audio] Failed to load background.mp3\n";
  }

  EmitterConfig earthCfg;
  earthCfg.emissionRate          = 100.f;
  earthCfg.maxParticles          = 1000;
  earthCfg.direction             = sf::degrees(0.f);
  earthCfg.directionVariance     = sf::degrees(360.f);
  earthCfg.speed                 = 30.f;
  earthCfg.speedVariance         = 10.f;
  earthCfg.startRotation         = sf::degrees(0.f);
  earthCfg.startRotationVariance = sf::degrees(360.f);
  earthCfg.angularVelocity       = 30.f;
  earthCfg.angularVelocityVariance = 15.f;
  earthCfg.gravity               = {0.f, 0.f};
  earthCfg.startColor            = sf::Color(100, 200, 255, 180);
  earthCfg.endColor              = sf::Color(100, 200, 255, 0);
  earthCfg.startSize             = {16.f, 16.f};
  earthCfg.endSize               = {0.f, 0.f};
  earthCfg.lifetime              = 3.0f;
  earthCfg.lifetimeVariance      = 0.3f;
  earthCfg.texture               = particleTex;
  earthCfg.textureAssetId        = particleId;
  earthCfg.blendMode             = sf::BlendAlpha;
  earthCfg.duration              = 0.f;
  earthCfg.loop                  = false;

  auto* earthParticles = earth->addComponent<ParticleSystemComponent>();
  earthParticles->setConfig(earthCfg);
  earthParticles->setSortMode(ParticleSortMode::kBackToFront);
  earthParticles->setWorldSpace(true);

  SceneNode* moon = scene.createNode("Moon", earth);
  moon->transform().setPosition({40.f, 0.f});
  moon->addComponent<CircleComponent>(4.f, sf::Color(100, 100, 100));
  auto* sfx = moon->addComponent<SourceComponent>();
  // A one-shot SFX is short and resident: cook it as a SoundAsset (kSound
  // backend) and reference it by UUID — unlike the streaming mp3 music above.
  sfx->setSoundAssetId(sfmx::UUID::createFromName("sfx.ogg"));
  sfx->setVolume(200.f);

  SceneNode* chinese = scene.createNode("Chinese");
  chinese->transform().setPosition({0.f, center.y});
  auto* cgm = chinese->addComponent<SourceComponent>();
  if (cgm->loadMusicFromFile("resources/chinese.mp3")) {
    cgm->setLooping(true);
    cgm->setVolume(10.f);
    cgm->setMinDistance(50.f);
    cgm->setAttenuation(0.3f);
    cgm->setSpatializationEnabled(true);
  }
  else {
    std::cout << "[Audio] Failed to load chinese.mp3\n";
  }

  SceneNode* mozart = scene.createNode("Mozart");
  mozart->transform().setPosition({windowWidth, center.y});
  auto* mgm = mozart->addComponent<SourceComponent>();
  if (mgm->loadMusicFromFile("resources/mozart.mp3")) {
    mgm->setLooping(true);
    mgm->setVolume(10.f);
    mgm->setPitch(2.0f);
    mgm->setMinDistance(50.f);
    mgm->setAttenuation(0.3f);
    mgm->setSpatializationEnabled(true);
  }
  else {
    std::cout << "[Audio] Failed to load mozart.mp3\n";
  }

  SceneNode* neptune = scene.createNode("Neptune", sun2);
  neptune->transform().setPosition({280.f, 100.f});
  neptune->addComponent<CircleComponent>(12.f, sf::Color(80, 100, 200));
  neptune->addComponent<ListenerComponent>();
  auto* sprite = neptune->addComponent<SpriteComponent>();
  sprite->setTextureAsset(loadTex("aleka.png"));  // sets texture + records the UUID
  sprite->setScale(0.1f);
  sprite->setOrigin({sprite->getPixelSize().x * 0.5f,
                     sprite->getPixelSize().y * 0.5f});
  sprite->setColor(sf::Color::White);

  // -- Mario atlas animation (frame rects auto-detected from the atlas image) --
  SceneNode* marioNode = scene.createNode("Mario");
  SPtr<TextureAsset> marioAsset = loadTex("marioatlas.png");
  if (nullptr != marioAsset && marioAsset->isLoaded()) {
    SPtr<sf::Texture> marioTex(marioAsset, &marioAsset->texture());
    const sf::Image marioImg = marioTex->copyToImage();

    auto rects = Atlas::detectSpriteRects(marioImg);
    rects.resize(6);  // take only the first frames
    SPtr<Animation> marioAnim = MakeShared<Animation>();
    marioAnim->m_loops = true;
    marioAnim->m_duration = static_cast<float>(rects.size()) * 0.1f;
    marioAnim->m_speedMultiplier = 1.0f;
    marioAnim->m_textureAssetId = marioAsset->metadata().uuid;

    for (const auto& r : rects) {
      marioAnim->m_frames.push_back({marioTex, r});
    }

    auto* marioAnimator = marioNode->addComponent<AnimatorComponent>();
    marioAnimator->addAnimation(marioAnim, "run");
    marioAnimator->play("run");  // selects the clip (serialized); resumed on load
  }
  else {
    std::cerr << "[SpriteAtlas] Failed to load marioatlas.png\n";
  }
  marioNode->transform().setPosition({0.f, 0.f});

  // -- Physics colliders --
  SceneNode* ground = scene.createNode("Ground");
  ground->transform().setPosition({center.x, windowHeight - 5.f});
  auto* groundCollider = ground->addComponent<ColliderComponent>();
  groundCollider->setAABB({windowWidth * 0.5f, 30.f});
  groundCollider->setDebugColor(sf::Color(100, 100, 100));

  auto spawnCollider = [&](sf::Vector2f pos, auto&& setupCollider) {
    SceneNode* n = scene.createNode("PhysObj");
    n->transform().setPosition(pos);
    auto* col = n->addComponent<ColliderComponent>();
    setupCollider(col);
    auto* rb = n->addComponent<RigidBodyComponent>();
    rb->setMass(1.f);
    rb->setGravityScale(1.f);
  };

  spawnCollider({center.x - 120.f, center.y}, [](ColliderComponent* c) {
    c->setCircle(16.f);
    c->setDebugColor(sf::Color::Red);
  });
  spawnCollider({center.x - 40.f, center.y}, [](ColliderComponent* c) {
    c->setAABB({15.f, 12.f});
    c->setDebugColor(sf::Color::Green);
  });
  spawnCollider({center.x + 40.f, center.y}, [](ColliderComponent* c) {
    c->setCircle(20.f);
    c->setDebugColor(sf::Color::Yellow);
  });
  spawnCollider({center.x + 120.f, center.y}, [](ColliderComponent* c) {
    c->setAABB({10.f, 18.f});
    c->setDebugColor(sf::Color::Cyan);
  });

  // -- Player: idle + walking animations (explicit frame rects) --
  auto* playerNode = scene.createNode("Player");
  playerNode->transform().setPosition({256.f, 256.f});
  auto* playerAnimator = playerNode->addComponent<AnimatorComponent>();

  SPtr<TextureAsset> idleAsset = loadTex("playeridle.png");
  if (nullptr != idleAsset && idleAsset->isLoaded()) {
    SPtr<sf::Texture> idleText(idleAsset, &idleAsset->texture());
    Vector<sf::IntRect> rects;
    rects.resize(4);
    rects[0] = {{0, 0},   {128, 128}};
    rects[1] = {{128, 0}, {128, 128}};
    rects[2] = {{256, 0}, {128, 128}};
    rects[3] = {{384, 0}, {128, 128}};
    SPtr<Animation> idleAnim = MakeShared<Animation>();
    idleAnim->m_loops = true;
    idleAnim->m_duration = static_cast<float>(rects.size()) * 0.5f;
    idleAnim->m_speedMultiplier = 1.0f;
    idleAnim->m_textureAssetId = idleAsset->metadata().uuid;

    for (const auto& r : rects) {
      idleAnim->m_frames.push_back({idleText, r});
    }

    playerAnimator->addAnimation(idleAnim, "idle");
    playerAnimator->play("idle");
  }
  else {
    std::cerr << "[SpriteAtlas] Failed to load playeridle.png\n";
  }

  SPtr<TextureAsset> walkingAsset = loadTex("playerwalking.png");
  if (nullptr != walkingAsset && walkingAsset->isLoaded()) {
    SPtr<sf::Texture> walkingText(walkingAsset, &walkingAsset->texture());
    Vector<sf::IntRect> rects;
    rects.resize(8);
    rects[0] = {{0, 0},   {128, 128}};
    rects[1] = {{128, 0}, {128, 128}};
    rects[2] = {{256, 0}, {128, 128}};
    rects[3] = {{384, 0}, {128, 128}};
    rects[4] = {{512, 0}, {128, 128}};
    rects[5] = {{640, 0}, {128, 128}};
    rects[6] = {{768, 0}, {128, 128}};
    rects[7] = {{896, 0}, {128, 128}};
    SPtr<Animation> walkingAnim = MakeShared<Animation>();
    walkingAnim->m_loops = true;
    walkingAnim->m_duration = static_cast<float>(rects.size()) * 0.1f;
    walkingAnim->m_speedMultiplier = 2.0f;
    walkingAnim->m_textureAssetId = walkingAsset->metadata().uuid;

    for (const auto& r : rects) {
      walkingAnim->m_frames.push_back({walkingText, r});
    }

    playerAnimator->addAnimation(walkingAnim, "walking");
    playerAnimator->play("walking");
  }
  else {
    std::cerr << "[SpriteAtlas] Failed to load playerwalking.png\n";
  }

  // -- Spaceship: Lua script --
  SceneNode* spaceship = scene.createNode("Spaceship");
  spaceship->transform().setPosition({center.x, 0.f});
  spaceship->addComponent<CircleComponent>(10.f, sf::Color(180, 180, 180));
  spaceship->addComponent<ScriptComponent>("resources/character.lua");
}

DemoRuntime
wireDemoRuntime(Scene& scene) {
  DemoRuntime rt;
  rt.sun     = firstByName(scene, "Sun");
  rt.sun2    = firstByName(scene, "Sun2");
  rt.earth   = firstByName(scene, "Earth");
  rt.neptune = firstByName(scene, "Neptune");

  // The scene's active camera is a runtime pointer, not serialized.
  if (SceneNode* cam = firstByName(scene, "Camera")) {
    if (auto* cameraComp = cam->getComponent<CameraComponent>()) {
      scene.setCamera(cameraComp);
    }
  }

  // Restart the looping music tracks (params survive serialization, play state
  // does not). The one-shot Moon sfx stays silent until Space.
  auto resumeMusic = [&](StringView nodeName) {
    if (SceneNode* n = firstByName(scene, nodeName)) {
      if (auto* src = n->getComponent<SourceComponent>()) {
        if (src->isLooping()) {
          src->play();
        }
      }
    }
  };
  resumeMusic("Earth");
  resumeMusic("Chinese");
  resumeMusic("Mozart");

  // Resume animators on their serialized selected clip (playback isn't serialized).
  auto resumeAnimator = [&](StringView nodeName) {
    if (SceneNode* n = firstByName(scene, nodeName)) {
      if (auto* anim = n->getComponent<AnimatorComponent>()) {
        anim->play();
      }
    }
  };
  resumeAnimator("Mario");
  resumeAnimator("Player");

  if (SceneNode* moon = firstByName(scene, "Moon")) {
    rt.moonSfx = moon->getComponent<SourceComponent>();
  }
  return rt;
}

} // namespace demo
