#pragma once

namespace sfmx {
class Scene;
class SceneNode;
class SourceComponent;
class MemoryPoolHandler;
} // namespace sfmx

namespace demo {

// Path of the cooked demo scene (repo-root relative, like the other assets).
inline constexpr const char* kSceneFile = "Game/assets/demo.sfmxasset";

// Node/component refs the game loop drives, resolved by name after the scene is
// ready (loaded or built).
struct DemoRuntime
{
  sfmx::SceneNode*       sun     = nullptr;
  sfmx::SceneNode*       sun2    = nullptr;
  sfmx::SceneNode*       earth   = nullptr;
  sfmx::SceneNode*       neptune = nullptr;
  sfmx::SourceComponent* moonSfx = nullptr;
};

// Pools the demo scene draws its nodes/components from. Needed by both the game
// and the cook path (createNode/addComponent allocate from these).
void
registerDemoPools(sfmx::MemoryPoolHandler& pools);

// ComponentRegistry factories so SceneSerializer::deserialize can rebuild the
// demo's components from their stored type ids. Game (load) path only.
void
registerDemoComponents();

// Build the demo scene in code: the authoring SOURCE the cooker serializes, and
// the game's fallback when the cooked file is missing. Scene DATA only — runtime
// behavior is wired separately by wireDemoRuntime().
void
buildDemoScene(sfmx::Scene& scene, float windowWidth, float windowHeight);

// Wire the runtime state the serialized scene does not carry (active camera,
// music/animation playback, the refs the game loop drives). Run after the scene
// is ready, uniformly for a loaded or a freshly-built scene.
DemoRuntime
wireDemoRuntime(sfmx::Scene& scene);

} // namespace demo
