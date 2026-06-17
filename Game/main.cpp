#include <SFML/Graphics.hpp>

#include "config/IniFile.h"
#include "input/ActionMap.h"
#include "input/Gamepad.h"
#include "input/InputAction.h"
#include "input/InputControl.h"
#include "input/InputSystem.h"
#include "input/Keyboard.h"
#include "input/Mapping.h"
#include "input/Mouse.h"
#include "scene/CameraComponent.h"
#include "scene/ListenerComponent.h"
#include "scene/ParticleSystemComponent.h"
#include "scene/Scene.h"
#include "scene/SourceComponent.h"
#include "scene/SpriteComponent.h"
#include "scene/AnimatorComponent.h"
#include "scene/ColliderComponent.h"
#include "scene/RigidBodyComponent.h"
#include "resource/SpriteAtlas.h"
#include "resource/Frame.h"
#include "utils/MemoryPoolHandler.h"
#include "utils/Module.h"
#include "utils/EventSystem.h"
#include "utils/Random.h"
#include "utils/Arithmetic.h"

#include "ui/UIEventSystem.h"
#include "ui/UITheme.h"
#include "ui/UIWidget.h"
#include "scene/UICanvasComponent.h"
#include "ui/UIPanel.h"
#include "ui/UILabel.h"
#include "ui/UIButton.h"
#include "ui/UIImage.h"
#include "ui/UICheckbox.h"
#include "ui/UISlider.h"
#include "ui/UITextBox.h"
#include "ui/UIHBox.h"
#include "ui/UIVBox.h"
#include "ui/UIEventSystem.h"
#include "input/Mapping.h"
#include "input/ActionMap.h"
#include "input/InputAction.h"
#include "input/InputControl.h"

using namespace sfmx;

class CircleComponent : public ComponentT<CircleComponent>
{
 public:
  CircleComponent(SceneNode* owner, float radius, sf::Color color)
    : ComponentT<CircleComponent>(owner)
  {
    m_circle.setRadius(radius);
    m_circle.setFillColor(color);
    m_circle.setOrigin({radius, radius});
  }

  void
  onDraw(sf::RenderTarget& target, sf::RenderStates states) const override {
    target.draw(m_circle, states);
  }

 private:
  sf::CircleShape m_circle;
};

DECLARE_TYPE_TRAITS(CircleComponent)
DECLARE_TYPE_TRAITS(SourceComponent)
DECLARE_TYPE_TRAITS(ListenerComponent)
DECLARE_TYPE_TRAITS(CameraComponent)
DECLARE_TYPE_TRAITS(AnimatorComponent)

int main()
{
  sfmx::IniFile config;
  config.loadAll({"Game/config/Engine.ini", "Game/config/Game.ini"});

  const uint32 windowWidth = config.getUInt("Window", "Width", 800u);
  const uint32 windowHeight = config.getUInt("Window", "Height", 600u);
  const String windowTitle = config.getString("Window", "Title", "SFMX Game");
  const bool enableVSync = config.getBool("Window", "VSync", true);

  sf::Vector2f center = {static_cast<float>(windowWidth) * 0.5f,
                         static_cast<float>(windowHeight) * 0.5f};

  sf::RenderWindow window(sf::VideoMode({windowWidth, windowHeight}), windowTitle);
  window.setVerticalSyncEnabled(enableVSync);

  InputSystem::startUp();
  UIEventSystem::startUp();
  PhysicsSystem::startUp();
  MemoryPoolHandler::startUp(4096);

  MemoryPoolHandler& pools = MemoryPoolHandler::instance();
  pools.registerPool<Particle>(2048);
  pools.registerPool<SceneNode>(1024);
  pools.registerPool<CircleComponent>(64);
  pools.registerPool<SourceComponent>(4);
  pools.registerPool<ListenerComponent>(1);
  pools.registerPool<ParticleSystemComponent>(8);
  pools.registerPool<SpriteComponent>(8);
  pools.registerPool<CameraComponent>(1);
  pools.registerPool<AnimatorComponent>(8);
  pools.registerPool<ColliderComponent>(64);
  pools.registerPool<RigidBodyComponent>(64);
  pools.registerPool<UICanvasComponent>(4);
  pools.registerPool<UIPanel>(8);
  pools.registerPool<UILabel>(16);
  pools.registerPool<UIButton>(8);
  pools.registerPool<UIImage>(8);
  pools.registerPool<UICheckbox>(8);
  pools.registerPool<UISlider>(8);
  pools.registerPool<UITextBox>(8);
  pools.registerPool<UIHBox>(8);
  pools.registerPool<UIVBox>(8);


  // ── UI Mapping ───────────────────────────────────────────────────────
  {
    Mapping* uiMapping = InputSystem::instance().createMapping("UI");
    InputSystem::instance().setActiveMapping(uiMapping);

    ActionMap* uiMap = uiMapping->addMap("UI");

    // Submit: Enter, Space, Gamepad South
    InputAction* submit = uiMap->addAction("Submit", ActionValueType::kButton);
    submit->addBinding({DeviceType::kKeyboard, Key::kEnter, -1, false});
    submit->addBinding({DeviceType::kKeyboard, Key::kSpace, -1, false});

    // Cancel: Escape, Gamepad East
    InputAction* cancel = uiMap->addAction("Cancel", ActionValueType::kButton);
    cancel->addBinding({DeviceType::kKeyboard, Key::kEscape, -1, false});

    UIEventSystem::instance().init(uiMapping);
  }

  Scene scene("Main");

  SceneNode* sun = scene.createNode("Sun");
  sun->transform().setPosition(center);
  sun->addComponent<CircleComponent>(40.f, sf::Color(255, 180, 100));
  
  auto* texture = new sf::Texture();
  if (!texture->loadFromFile("Game/resources/particle.png"))
  {
    std::cerr << "Failed loading particle texture\n";
    delete texture;
    return -1;
  }

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
  sunCfg.texture               = texture;
  sunCfg.blendMode             = sf::BlendAlpha;
  sunCfg.duration              = 0.f;
  sunCfg.loop                  = false;

  auto* sunParticles = sun->addComponent<ParticleSystemComponent>();
  sunParticles->setConfig(sunCfg);
  sunParticles->setSortMode(ParticleSortMode::BackToFront);
  sunParticles->setWorldSpace(false);

  SceneNode* sun2 = scene.createNode("Sun2");
  sun2->transform().setPosition(center);

  SceneNode* cameraNode = scene.createNode("Camera", sun);
  auto* camera = cameraNode->addComponent<CameraComponent>();
  camera->setSize({static_cast<float>(windowWidth) * 2.f,
                   static_cast<float>(windowHeight) * 2.f});
  camera->setFollowNode(true);
  scene.setCamera(camera);
  
  SceneNode* earth = scene.createNode("Earth", sun);
  earth->transform().setPosition({140.f, 0.f});
  earth->addComponent<CircleComponent>(20.f, sf::Color(100, 180, 255));
  auto* bgm = earth->addComponent<SourceComponent>();
  {
    if (bgm->loadMusicFromFile("Game/resources/background.mp3")) {
      bgm->setLooping(true);
      bgm->setVolume(1.0f);
      bgm->setSpatializationEnabled(false);
      bgm->play();
    }
    else {
      std::cout << "[Audio] Failed to load background.mp3\n";
    }
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
  earthCfg.texture               = texture;
  earthCfg.blendMode             = sf::BlendAlpha;
  earthCfg.duration              = 0.f;
  earthCfg.loop                  = false;

  auto* earthParticles = earth->addComponent<ParticleSystemComponent>();
  earthParticles->setConfig(earthCfg);
  earthParticles->setSortMode(ParticleSortMode::BackToFront);
  earthParticles->setWorldSpace(true);

  SceneNode* moon = scene.createNode("Moon", earth);
  moon->transform().setPosition({40.f, 0.f});
  moon->addComponent<CircleComponent>(4.f, sf::Color(100, 100, 100));
  SourceComponent* moonSfx = nullptr;
  auto* sfx = moon->addComponent<SourceComponent>();
  {
    if (sfx->loadMusicFromFile("Game/resources/sfx.mp3")) {
      sfx->setVolume(200.f);
    }
    else {
      std::cout << "[Audio] Failed to load sfx.mp3\n";
    }
    moonSfx = sfx;
  }

  SceneNode* chinese = scene.createNode("Chinese");
  chinese->transform().setPosition({0, center.y});
  auto* cgm = chinese->addComponent<SourceComponent>();
  {
    if (cgm->loadMusicFromFile("Game/resources/chinese.mp3")) {
      cgm->setLooping(true);
      cgm->setVolume(10.f);
      cgm->setMinDistance(50.f);
      cgm->setAttenuation(0.3f);
      cgm->setSpatializationEnabled(true);
      cgm->play();
    }
    else {
      std::cout << "[Audio] Failed to load chinese.mp3\n";
    }
  }
  SceneNode* mozart = scene.createNode("Mozart");
  mozart->transform().setPosition({static_cast<float>(windowWidth), center.y});
  auto* mgm = mozart->addComponent<SourceComponent>();
  {
    if (mgm->loadMusicFromFile("Game/resources/mozart.mp3")) {
      mgm->setLooping(true);
      mgm->setVolume(10.f);
      mgm->setPitch(2.0f);
      mgm->setMinDistance(50.f);
      mgm->setAttenuation(0.3f);
      mgm->setSpatializationEnabled(true);
      mgm->play();
    } else {
      std::cout << "[Audio] Failed to load mozart.mp3\n";
    }
  }

  SceneNode* neptune = scene.createNode("Neptune", sun2);
  neptune->transform().setPosition({280.f, 100.f});
  neptune->addComponent<CircleComponent>(12.f, sf::Color(80, 100, 200));
  neptune->addComponent<ListenerComponent>();
  auto* sprite = neptune->addComponent<SpriteComponent>();
  auto* texture2 = new sf::Texture();
  if (!texture2->loadFromFile("Game/resources/aleka.png"))
  {
    std::cerr << "Failed loading particle texture\n";
    delete texture2;
    return -1;
  }
  sprite->setTexture(*texture2);
  sprite->setScale(0.1f);
  sprite->setOrigin({sprite->getPixelSize().x * 0.5f, 
                     sprite->getPixelSize().y * 0.5f});
  sprite->setColor(sf::Color::White);

  // --- Mario atlas animation demo ---

  SceneNode* marioNode = scene.createNode("Mario");
  SPtr<sf::Texture> marioTex = MakeShared<sf::Texture>();
  if (marioTex->loadFromFile("Game/resources/marioatlas.png"))
  {
    const sf::Image marioImg = marioTex->copyToImage();

    auto rects = Atlas::detectSpriteRects(marioImg);
    int actualFrames = 6; // Take only the first 12 frames
    rects.resize(actualFrames);
    std::cout << "[SpriteAtlas] Detected " << rects.size() << " frames\n";
    SPtr<Animation> marioAnim = MakeShared<Animation>();
    marioAnim->m_loops = true;
    marioAnim->m_duration = static_cast<float>(rects.size()) * 0.1f;
    marioAnim->m_speedMultiplier = 1.0f;

    for (const auto& r : rects) {
      marioAnim->m_frames.push_back({marioTex, r});
    }

    auto* marioAnimator = marioNode->addComponent<AnimatorComponent>();
    marioAnimator->addAnimation(marioAnim, "run");
    marioAnimator->play("run");
  }
  else
  {
    std::cerr << "[SpriteAtlas] Failed to load marioatlas.png\n";
  }
  
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

  auto* marioSprite = marioNode->getComponent<SpriteComponent>();
  // marioSprite->setOrigin({marioSprite->getPixelSize().x * 0.5f,
  //                         marioSprite->getPixelSize().y * 0.5f});
  marioNode->transform().setPosition({0,0});

  auto* playerNode = scene.createNode("Player");
  playerNode->transform().setPosition({256,256});
  auto* playerAnimator = playerNode->addComponent<AnimatorComponent>();
  SPtr<sf::Texture> idleText = MakeShared<sf::Texture>();
  if (idleText->loadFromFile("Game/resources/playeridle.png"))
  {
    Vector<sf::IntRect> rects;
    rects.resize(4);
    rects[0] = {{0,0}, {128, 128}};
    rects[1] = {{128,0}, {128, 128}};
    rects[2] = {{256,0}, {128, 128}};
    rects[3] = {{384,0}, {128, 128}};
    // int actualFrames = 6; // Take only the first 12 frames
    // rects.resize(actualFrames);
    std::cout << "[SpriteAtlas] Detected " << rects.size() << " frames\n";
    SPtr<Animation> idleAnim = MakeShared<Animation>();
    idleAnim->m_loops = true;
    idleAnim->m_duration = static_cast<float>(rects.size()) * 0.5f;
    idleAnim->m_speedMultiplier = 1.0f;

    for (const auto& r : rects) {
      idleAnim->m_frames.push_back({idleText, r});
    }

    playerAnimator->addAnimation(idleAnim, "idle");
    playerAnimator->play("idle");
  }
  else
  {
    std::cerr << "[SpriteAtlas] Failed to load playeridle.png\n";
  }
  SPtr<sf::Texture> walkingText = MakeShared<sf::Texture>();
  if (walkingText->loadFromFile("Game/resources/playerwalking.png"))
  {
    Vector<sf::IntRect> rects;
    rects.resize(8);
    rects[0] = {{0,0}, {128, 128}};
    rects[1] = {{128,0}, {128, 128}};
    rects[2] = {{256,0}, {128, 128}};
    rects[3] = {{384,0}, {128, 128}};
    rects[4] = {{512,0}, {128, 128}};
    rects[5] = {{640,0}, {128, 128}};
    rects[6] = {{768,0}, {128, 128}};
    rects[7] = {{896,0}, {128, 128}};
    // int actualFrames = 6; // Take only the first 12 frames
    // rects.resize(actualFrames);
    std::cout << "[SpriteAtlas] Detected " << rects.size() << " frames\n";
    SPtr<Animation> walkingAnim = MakeShared<Animation>();
    walkingAnim->m_loops = true;
    walkingAnim->m_duration = static_cast<float>(rects.size()) * 0.1f;
    walkingAnim->m_speedMultiplier = 2.0f;

    for (const auto& r : rects) {
      walkingAnim->m_frames.push_back({walkingText, r});
    }

    playerAnimator->addAnimation(walkingAnim, "walking");
    playerAnimator->play("walking");
  }
  else
  {
    std::cerr << "[SpriteAtlas] Failed to load playerwalking.png\n";
  }
  

  // InputSystem: Example of "Mapping Mode", you create a "Mapping", which
  // contains a "Map", a map contains "Actions", an action contains "Bindings",
  // a binding defines what inputs trigger the action, it also has an
  // "Interaction" type, and "Processors", an interaction defines the conditions
  // of the input to trigger the action, and the processor is a modification to
  // the input value before paasing it to the callback.
  // Demo: Jump (tap), Crouch (hold), Move (passthrough Vector2, normalized).
  Mapping* controls = InputSystem::instance().createMapping("DefaultControls");
  ActionMap* gameplay = controls->addMap("Gameplay");

  InputAction* jump = gameplay->addAction("Jump", ActionValueType::kButton);
  jump->addBinding(InputControl{DeviceType::kKeyboard, Key::kSpace, -1, false});
  Interaction tap;
  tap.m_type = InteractionType::kTap;
  tap.m_duration = 0.2f;
  jump->setInteraction(tap);

  InputAction* crouch = gameplay->addAction("Crouch", ActionValueType::kButton);
  crouch->addBinding(InputControl{DeviceType::kKeyboard, Key::kLControl, -1, false});
  Interaction hold;
  hold.m_type = InteractionType::kHold;
  hold.m_duration = 0.4f;
  crouch->setInteraction(hold);

  InputAction* move = gameplay->addAction("Move", ActionValueType::kAxis2D);
  CompositeBinding& moveComposite = move->addComposite(CompositeType::kVector2D);
  moveComposite.m_parts.push_back(
    {InputControl{DeviceType::kKeyboard, Key::kA, -1, false}, CompositeRole::kNegativeX, {}});
  moveComposite.m_parts.push_back(
    {InputControl{DeviceType::kKeyboard, Key::kD, -1, false}, CompositeRole::kPositiveX, {}});
  moveComposite.m_parts.push_back(
    {InputControl{DeviceType::kKeyboard, Key::kS, -1, false}, CompositeRole::kNegativeY, {}});
  moveComposite.m_parts.push_back(
    {InputControl{DeviceType::kKeyboard, Key::kW, -1, false}, CompositeRole::kPositiveY, {}});
  move->addProcessor(Processor{ProcessorType::kNormalize, {}, {}});

  InputSystem::instance().setActiveMapping(controls);

  HEvent jumpSub = jump->onPerformed([](const InputContext&) {
    std::cout << "[Action] Jump performed\n";
  });
  HEvent crouchSub = crouch->onPerformed([](const InputContext&) {
    std::cout << "[Action] Crouch performed (held past threshold)\n";
  });
  HEvent crouchStart = crouch->onStarted([](const InputContext&) {
    std::cout << "[Action] Crouch started\n";
  });
  HEvent crouchEnd = crouch->onCanceled([](const InputContext&) {
    std::cout << "[Action] Crouch canceled\n";
  });
  float moveReportTimer = 0.1f;
  HEvent moveSub = move->onPerformed([&moveReportTimer](const InputContext& ctx) {
    // Performed fires every non-zero frame; throttle the print.
    moveReportTimer += ctx.m_deltaTime;
    if (moveReportTimer >= 0.1f) {
      const Vector2f value = ctx.m_value.asVector2();
      std::cout << "[Action] Move (" << value.x << ", " << value.y << ")\n";
      moveReportTimer = 0.f;
    }
  });

  sf::Clock clock;
  std::cout << "Random demo:\n"
            << "Get: "    << Random::get<float>()       << "\n"
            << "Get: "    << Random::get<float>()       << "\n"
            << "Get: "    << Random::get<float>()       << "\n"
            << "Range: "  << Random::range<int>(0, 30)  << "\n"
            << "Range: "  << Random::range<int>(0, 30)  << "\n"
            << "Range: "  << Random::range<int>(0, 30)  << "\n"
            << "Dice: "   << Random::diceThrow(3, 6)    << "\n"
            << "Dice: "   << Random::diceThrow(2, 6)    << "\n"
            << "Dice: "   << Random::diceThrow(1, 6)    << "\n";

  SceneNode* uiRoot = scene.createNode("UIRoot");
  auto* canvas = uiRoot->addComponent<UICanvasComponent>();
  UIEventSystem::instance().registerCanvas(canvas);
  canvas->setCanvasSize({static_cast<float>(windowWidth),
                         static_cast<float>(windowHeight)});
  SPtr<sf::Font> uiFont = MakeShared<sf::Font>();
  if (!uiFont->openFromFile("/usr/share/fonts/TTF/Hack-Regular.ttf")) {
    std::cerr << "[UI] Failed to load font\n";
  }

  // SceneNode* bgNode = scene.createNode("UIBg", uiRoot);
  // auto* bg = bgNode->addComponent<UIPanel>();
  // bg->setSize({static_cast<float>(windowWidth), static_cast<float>(windowHeight)});
  // bg->setFillColor({30, 30, 35});
  // bg->setBorderWidth(0.f);
// 
  SceneNode* titleNode = scene.createNode("UITitle", uiRoot);
  auto* title = titleNode->addComponent<UILabel>();
  title->setAnchors(Anchors::center());
  title->setSize({400.f, 60.f});
  title->setPivot({0.5f, 0.5f});
  title->setText("SFMX UI Demo");
  title->setFont(uiFont);
  title->setFontSize(36);
  title->setTextColor({200, 200, 210});
  title->setOffset({0.f, -60.f});
// 
  SceneNode* btnNode = scene.createNode("UIButton", uiRoot);
  auto* btn = btnNode->addComponent<UIButton>();
  btn->setAnchors(Anchors::center());
  btn->setSize({200.f, 50.f});
  btn->setPivot({0.5f, 1.0f});
  btn->setFont(uiFont);
  btn->setFontSize(18);
  btn->setText("Click Me");
  btn->setFillColor({70, 100, 180});
  btn->setHoverColor({90, 120, 210});
  btn->setPressColor({50, 80, 160});
  btn->setOffset({0.f, 60.f});
  btn->setFocusable(true);
  [[maybe_unused]] HEvent clickSub = btn->onClick().connect([btn] {
    std::cout << "[UI] Button clicked!\n";
    btn->setText("Clicked!");
  });

  // ── Checkbox ───────────────────────────────────────────────────────────
  SceneNode* chkNode = scene.createNode("UICheckbox", uiRoot);
  auto* chk = chkNode->addComponent<UICheckbox>();
  chk->setAnchors(Anchors::center());
  chk->setSize({200.f, 30.f});
  chk->setPivot({0.5f, 0.5f});
  chk->setOffset({0.f, 130.f});
  chk->setFont(uiFont);
  chk->setText("Enable feature");
  chk->setFontSize(16);
  [[maybe_unused]] HEvent toggleSub = chk->onToggle().connect([](bool v) {
    std::cout << "[UI] Checkbox toggled: " << (v ? "ON" : "OFF") << "\n";
  });

  // ── Slider ─────────────────────────────────────────────────────────────
  SceneNode* sldNode = scene.createNode("UISlider", uiRoot);
  auto* sld = sldNode->addComponent<UISlider>();
  sld->setAnchors(Anchors::center());
  sld->setSize({250.f, 30.f});
  sld->setPivot({0.5f, 0.5f});
  sld->setOffset({0.f, 180.f});
  sld->setRange(0.f, 100.f);
  sld->setValue(50.f);
  [[maybe_unused]] HEvent sliderSub = sld->onValueChanged().connect([](float v) {
    std::cout << "[UI] Slider value: " << v << "\n";
  });

  // ── TextBox ────────────────────────────────────────────────────────────
  SceneNode* tbNode = scene.createNode("UITextBox", uiRoot);
  auto* tb = tbNode->addComponent<UITextBox>();
  tb->setAnchors(Anchors::center());
  tb->setSize({300.f, 36.f});
  tb->setPivot({0.5f, 0.5f});
  tb->setOffset({0.f, 230.f});
  tb->setFont(uiFont);
  tb->setFontSize(16);
  tb->setPlaceholder("Type here...");
  [[maybe_unused]] HEvent textSub = tb->onTextChanged().connect([](const sf::String& t) {
    std::cout << "[UI] TextBox: \"" << t.toAnsiString() << "\"\n";
  });

  SceneNode* tb2Node = scene.createNode("UITextBox2", uiRoot);
  auto* tb2 = tb2Node->addComponent<UITextBox>();
  tb2->setAnchors(Anchors::center());
  tb2->setSize({300.f, 36.f});
  tb2->setPivot({0.5f, 0.5f});
  tb2->setOffset({0.f, 280.f});
  tb2->setFont(uiFont);
  tb2->setFontSize(16);
  tb2->setPlaceholder("Second field... (Tab here)");

  // ── Image ──────────────────────────────────────────────────────────────
  // Build a small checkerboard texture to demonstrate UIImage.
  auto imgTex = MakeShared<sf::Texture>();
  {
    sf::Image img(sf::Vector2u{64, 64}, sf::Color::White);
    for (unsigned y = 0; y < 64; ++y)
      for (unsigned x = 0; x < 64; ++x)
        if ((x / 8 + y / 8) % 2 == 0)
          img.setPixel({x, y}, sf::Color{100, 140, 220});
    [[maybe_unused]] bool loaded = imgTex->loadFromImage(img);
    (void)loaded;
  }

  SceneNode* imgNode = scene.createNode("UIImage", uiRoot);
  auto* img = imgNode->addComponent<UIImage>();
  img->setAnchors(Anchors::center());
  img->setSize({80.f, 80.f});
  img->setPivot({0.5f, 0.5f});
  img->setOffset({0.f, -140.f});
  img->setTexture(imgTex);

  // ── HBox container: 3 buttons in a row ───────────────────────────────
  SceneNode* hboxNode = scene.createNode("UIHBox", uiRoot);
  auto* hbox = hboxNode->addComponent<UIHBox>();
  hbox->setAnchors(Anchors::bottomLeft());
  hbox->setSize({static_cast<float>(windowWidth), 50.f});
  hbox->setPivot({0.f, 1.f});
  hbox->setOffset({0.f, -10.f});
  hbox->setPadding(10.f);
  hbox->setSpacing(12.f);
  hbox->setChildAlign(0.5f);

  auto makeBoxBtn = [&](const char* label, sf::Color fill, sf::Color hover, sf::Color press) -> UIButton* {
    SceneNode* n = scene.createNode("HBoxBtn", hboxNode);
    auto* b = n->addComponent<UIButton>();
    b->setSize({120.f, 36.f});
    b->setFont(uiFont);
    b->setFontSize(15);
    b->setText(label);
    b->setFillColor(fill);
    b->setHoverColor(hover);
    b->setPressColor(press);
    return b;
  };

  [[maybe_unused]] UIButton* hBtn1 = makeBoxBtn("Play",    {70, 160, 80},  {90, 180, 100}, {50, 140, 60});
  [[maybe_unused]] UIButton* hBtn2 = makeBoxBtn("Settings", {70, 100, 180}, {90, 120, 210}, {50, 80, 160});
  [[maybe_unused]] UIButton* hBtn3 = makeBoxBtn("Quit",    {180, 70, 70},  {210, 90, 90},  {150, 50, 50});

  // Explicit navigation between HBox buttons
  hBtn1->setNavigationMode(NavigationMode::Explicit);
  hBtn1->setNavRight(hBtn2);
  hBtn2->setNavigationMode(NavigationMode::Explicit);
  hBtn2->setNavLeft(hBtn1);
  hBtn2->setNavRight(hBtn3);
  hBtn3->setNavigationMode(NavigationMode::Explicit);
  hBtn3->setNavLeft(hBtn2);

  // ── VBox container: label + slider + checkbox ─────────────────────────
  SceneNode* vboxNode = scene.createNode("UIVBox", uiRoot);
  auto* vbox = vboxNode->addComponent<UIVBox>();
  vbox->setAnchors(Anchors::topRight());
  vbox->setSize({220.f, 160.f});
  vbox->setPivot({1.f, 0.f});
  vbox->setOffset({-10.f, 10.f});
  vbox->setPadding(8.f);
  vbox->setSpacing(6.f);
  vbox->setChildAlign(0.f); // top-align

  {
    SceneNode* vlblNode = scene.createNode("VBoxLabel", vboxNode);
    auto* vlbl = vlblNode->addComponent<UILabel>();
    vlbl->setSize({0.f, 24.f});
    vlbl->setFont(uiFont);
    vlbl->setFontSize(14);
    vlbl->setText("Volume Settings");
    vlbl->setTextColor({200, 200, 210});

    SceneNode* vsldNode = scene.createNode("VBoxSlider", vboxNode);
    auto* vsld = vsldNode->addComponent<UISlider>();
    vsld->setSize({0.f, 24.f});
    vsld->setRange(0.f, 100.f);
    vsld->setValue(75.f);

    SceneNode* vchkNode = scene.createNode("VBoxCheck", vboxNode);
    auto* vchk = vchkNode->addComponent<UICheckbox>();
    vchk->setSize({0.f, 24.f});
    vchk->setFont(uiFont);
    vchk->setFontSize(14);
    vchk->setText("Mute");
  }

  // ── Instructions ──────────────────────────────────────────────────────
  {
    SceneNode* hintNode = scene.createNode("UIHint", uiRoot);
    auto* hint = hintNode->addComponent<UILabel>();
    hint->setAnchors(Anchors::center());
    hint->setSize({600.f, 24.f});
    hint->setPivot({0.5f, 0.5f});
    hint->setOffset({0.f, 340.f});
    hint->setFont(uiFont);
    hint->setFontSize(13);
    hint->setText("Arrow keys / gamepad navigate     Enter / Space activate     Click unfocus");
    hint->setTextColor({140, 140, 150});
  }

  // ── UI navigation setup ──────────────────────────────────────────────
  UIEventSystem::instance().setFirstSelected(hBtn1);
  UIEventSystem::instance().setNavigationWrap(true);

  while (window.isOpen())
  {
    // InputSystem: snapshot device state before polling
    InputSystem::instance().beginFrame();
    UIEventSystem::instance().beginFrame();

    while (const Optional<sf::Event> event = window.pollEvent())
    {
      if (event->is<sf::Event::Closed>())
      {
        window.close();
      }

      InputSystem::instance().onEvent(*event);

      if (const auto* text = event->getIf<sf::Event::TextEntered>())
        InputSystem::instance().pushTextCharacter(text->unicode);
    }

    const float deltaTime = clock.restart().asSeconds();
    
    InputSystem::instance().update(deltaTime, window);

    if (Keyboard::instance().wasPressedThisFrame(Key::kEscape)) {
      window.close();
    }
    if (Keyboard::instance().wasPressedThisFrame(Key::kSpace)) {
      std::cout << "[Input] Space pressed\n";
      moonSfx->stop();
      moonSfx->play();
    }
    if (Mouse::instance().wasPressedThisFrame(MouseButton::kLeft)) {
      const Vector2i pos = Mouse::instance().getPosition();
      std::cout << "[Input] Left click at (" << pos.x << ", " << pos.y << ")\n";
    }
    if (Gamepad::instance().isConnected(0)) {
      const float lx = Gamepad::instance().get(0).getAxis(Axis::kLeftX);
      if (lx != 0.f) {
        std::cout << "[Input] Gamepad 0 LeftX = " << lx << "\n";
      }
    }

    // Rotating the parent drags the child with it: proof of transform
    // composition down the hierarchy.
    sun->transform().rotate(sf::degrees(45.f * deltaTime));
    sun2->transform().rotate(sf::degrees(10.f * deltaTime));
    earth->transform().rotate(sf::degrees(215.f * deltaTime));
    neptune->transform().rotate(sf::degrees(-15.f * deltaTime));

    scene.update(deltaTime);

    window.clear(sf::Color(24, 24, 28));

    uiRoot->setVisible(false);
    scene.draw(window);
    uiRoot->setVisible(true);
    uiRoot->draw(window, sf::RenderStates::Default);

    window.display();
  }

  UIEventSystem::shutDown();
  InputSystem::shutDown();
  PhysicsSystem::shutDown();
  // Tear down pools last: ~Scene only drops ids/registry, so the pooled nodes
  // and components are destroyed here, while SFML is still alive.
  MemoryPoolHandler::shutDown();

  return 0;
}