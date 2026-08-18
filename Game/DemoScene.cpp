#include "DemoScene.h"

#include <SFML/Graphics.hpp>

#include "scene/AnimatorComponent.h"
#include "scene/CameraComponent.h"
#include "scene/CanvasComponent.h"
#include "scene/ColliderComponent.h"
#include "scene/ComponentRegistry.h"
#include "scene/ListenerComponent.h"
#include "scene/MaterialComponent.h"
#include "scene/ParticleSystemComponent.h"
#include "scene/RigidBodyComponent.h"
#include "scene/Scene.h"
#include "scene/ScriptComponent.h"
#include "scene/SourceComponent.h"
#include "scene/SpriteComponent.h"
#include "ui/UIButton.h"
#include "ui/UICheckbox.h"
#include "ui/UIHorizontalBox.h"
#include "ui/UIImage.h"
#include "ui/UILabel.h"
#include "ui/UIScrollView.h"
#include "ui/UISlider.h"
#include "ui/UITextBox.h"
#include "ui/UIVerticalBox.h"

#include "resource/Frame.h"
#include "resource/SpriteAtlas.h"

#include "utils/MemoryPoolHandler.h"

#include "assets/AssetManager.h"
#include "assets/ShaderAsset.h"
#include "assets/TextureAsset.h"

#include "core/DataStream.h"
#include "core/DataStreamTypes.h"

#include <iostream>

using namespace sfmx;

// Debug-viz component: a filled circle drawn at the node. Serializable (radius
// + color) so it round-trips through the cooker. Created at level-build and
// long-lived, so its sf::CircleShape allocates once at construction (setup, not
// per-frame churn) — acceptable for a build-time object.
class CircleComponent : public ComponentT<CircleComponent> {
public:
  explicit CircleComponent(SceneNode *owner)
      : ComponentT<CircleComponent>(owner) {}

  CircleComponent(SceneNode *owner, float radius, sf::Color color)
      : ComponentT<CircleComponent>(owner) {
    setCircle(radius, color);
  }

  void setCircle(float radius, sf::Color color) {
    m_circle.setRadius(radius);
    m_circle.setFillColor(color);
    m_circle.setOrigin({radius, radius});
  }

  void onDraw(sf::RenderTarget &target,
              sf::RenderStates states) const override {
    target.draw(m_circle, states);
  }

  void onSerialize(DataStream &stream) const override {
    stream << static_cast<uint32>(1); // version
    stream << m_circle.getRadius();
    const sf::Color color = m_circle.getFillColor();
    stream << color.r << color.g << color.b << color.a;
  }

  void onDeserialize(DataStream &stream) override {
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

// Resolve a cooked texture by its source-relative name (the id the cooker
// used).
SPtr<TextureAsset> loadTex(const ansichar *rel) {
  SPtr<TextureAsset> asset = AssetManager::instance().load<TextureAsset>(
      sfmx::UUID::createFromName(rel));
  if (nullptr == asset) {
    std::cerr << "[Assets] missing: " << rel << " (a build runs --cook)\n";
  }
  return asset;
}

SceneNode *firstByName(Scene &scene, StringView name) {
  Vector<SceneNode *> nodes = scene.findNodesByName(name);
  return nodes.empty() ? nullptr : nodes.front();
}

} // namespace

void registerDemoPools(MemoryPoolHandler &pools) {
  pools.registerPool<SceneNode>(1024 * 100);
  pools.registerPool<CircleComponent>(64);
  pools.registerPool<SourceComponent>(4);
  pools.registerPool<ListenerComponent>(1);
  pools.registerPool<CameraComponent>(1);
  pools.registerPool<SpriteComponent>(1024 * 100);
  pools.registerPool<MaterialComponent>(256);
  pools.registerPool<AnimatorComponent>(256);
  pools.registerPool<Particle>(1024 * 100);
  pools.registerPool<ParticleSystemComponent>(64);
  pools.registerPool<ColliderComponent>(64);
  pools.registerPool<RigidBodyComponent>(64);
  pools.registerPool<ScriptComponent>(1024 * 100);
  pools.registerPool<UIButton>(64);
  pools.registerPool<UILabel>(64);
  pools.registerPool<UIImage>(64);
  pools.registerPool<UICheckbox>(64);
  pools.registerPool<UITextBox>(64);
  pools.registerPool<UISlider>(64);
  pools.registerPool<UIVerticalBox>(16);
  pools.registerPool<UIHorizontalBox>(16);
  pools.registerPool<UIScrollView>(16);
  pools.registerPool<CanvasComponent>(8);

  std::cout << "Total pools memory usage: " << pools.getTotalMemoryUsage()
            << "\n";
  std::cout << "[Info] SceneNode pool memory usage: "
            << pools.pool<SceneNode>().getMemoryUsage() << std::endl;
  std::cout << "[Info] CircleComponent pool memory usage: "
            << pools.pool<CircleComponent>().getMemoryUsage() << std::endl;
  std::cout << "[Info] SourceComponent pool memory usage: "
            << pools.pool<SourceComponent>().getMemoryUsage() << std::endl;
  std::cout << "[Info] ListenerComponent pool memory usage: "
            << pools.pool<ListenerComponent>().getMemoryUsage() << std::endl;
  std::cout << "[Info] CameraComponent pool memory usage: "
            << pools.pool<CameraComponent>().getMemoryUsage() << std::endl;
  std::cout << "[Info] SpriteComponent pool memory usage: "
            << pools.pool<SpriteComponent>().getMemoryUsage() << std::endl;
  std::cout << "[Info] AnimatorComponent pool memory usage: "
            << pools.pool<AnimatorComponent>().getMemoryUsage() << std::endl;
  std::cout << "[Info] Particle pool memory usage: "
            << pools.pool<Particle>().getMemoryUsage() << std::endl;
  std::cout << "[Info] ParticleSystemComponent pool memory usage: "
            << pools.pool<ParticleSystemComponent>().getMemoryUsage()
            << std::endl;
  std::cout << "[Info] ColliderComponent pool memory usage: "
            << pools.pool<ColliderComponent>().getMemoryUsage() << std::endl;
  std::cout << "[Info] RigidBodyComponent pool memory usage: "
            << pools.pool<RigidBodyComponent>().getMemoryUsage() << std::endl;
  std::cout << "[Info] ScriptComponent pool memory usage: "
            << pools.pool<ScriptComponent>().getMemoryUsage() << std::endl;
  std::cout << "[Info] UIButton pool memory usage: "
            << pools.pool<UIButton>().getMemoryUsage() << std::endl;
  std::cout << "[Info] UILabel pool memory usage: "
            << pools.pool<UILabel>().getMemoryUsage() << std::endl;
  std::cout << "[Info] UIImage pool memory usage: "
            << pools.pool<UIImage>().getMemoryUsage() << std::endl;
  std::cout << "[Info] CanvasComponent pool memory usage: "
            << pools.pool<CanvasComponent>().getMemoryUsage() << std::endl;
}

void registerDemoComponents() {
  ComponentRegistry &reg = ComponentRegistry::instance();
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
  reg.registerComponent<UIVerticalBox>();
  reg.registerComponent<UIHorizontalBox>();
  reg.registerComponent<UIScrollView>();
}

void poolsInfo() {
  MemoryPoolHandler &pools = MemoryPoolHandler::instance();

  std::cout << "[Info] Total SceneNode elements: "
            << pools.pool<SceneNode>().getAllocatedCount() << std::endl;
  std::cout << "[Info] Total CircleComponent elements: "
            << pools.pool<CircleComponent>().getAllocatedCount() << std::endl;
  std::cout << "[Info] Total SourceComponent elements: "
            << pools.pool<SourceComponent>().getAllocatedCount() << std::endl;
  std::cout << "[Info] Total ListenerComponent elements: "
            << pools.pool<ListenerComponent>().getAllocatedCount() << std::endl;
  std::cout << "[Info] Total CameraComponent elements: "
            << pools.pool<CameraComponent>().getAllocatedCount() << std::endl;
  std::cout << "[Info] Total SpriteComponent elements: "
            << pools.pool<SpriteComponent>().getAllocatedCount() << std::endl;
  std::cout << "[Info] Total AnimatorComponent elements: "
            << pools.pool<AnimatorComponent>().getAllocatedCount() << std::endl;
  std::cout << "[Info] Total Particle elements: "
            << pools.pool<Particle>().getAllocatedCount() << std::endl;
  std::cout << "[Info] Total ParticleSystemComponent elements: "
            << pools.pool<ParticleSystemComponent>().getAllocatedCount()
            << std::endl;
  std::cout << "[Info] Total ColliderComponent elements: "
            << pools.pool<ColliderComponent>().getAllocatedCount() << std::endl;
  std::cout << "[Info] Total RigidBodyComponent elements: "
            << pools.pool<RigidBodyComponent>().getAllocatedCount()
            << std::endl;
  std::cout << "[Info] Total ScriptComponent elements: "
            << pools.pool<ScriptComponent>().getAllocatedCount() << std::endl;
  std::cout << "[Info] Total UIButton elements: "
            << pools.pool<UIButton>().getAllocatedCount() << std::endl;
  std::cout << "[Info] Total UILabel elements: "
            << pools.pool<UILabel>().getAllocatedCount() << std::endl;
  std::cout << "[Info] Total UIImage elements: "
            << pools.pool<UIImage>().getAllocatedCount() << std::endl;
  std::cout << "[Info] Total CanvasComponent elements: "
            << pools.pool<CanvasComponent>().getAllocatedCount() << std::endl;
}

void buildDemoScene(Scene &scene, float windowWidth, float windowHeight) {
  const sf::Vector2f center = {windowWidth * 0.5f, windowHeight * 0.5f};

  SceneNode *cameraNode = scene.createNode("Camera");
  auto *camera = cameraNode->addComponent<CameraComponent>();
  camera->setSize({windowWidth, windowHeight});
  camera->setFollowNode(true);
}

DemoRuntime wireDemoRuntime(Scene &scene) {
  DemoRuntime rt;

  // The scene's active camera is a runtime pointer, not serialized.
  if (SceneNode *cam = firstByName(scene, "Camera")) {
    if (auto *cameraComp = cam->getComponent<CameraComponent>()) {
      scene.setCamera(cameraComp);
    }
  }

  return rt;
}

} // namespace demo
