/*
#include <SFML/Graphics.hpp>

#include "config/IniFile.h"

#include "core/platform/PlatformTypes.h"
#include "input/ActionMap.h"
#include "input/Gamepad.h"
#include "input/InputAction.h"
#include "input/InputControl.h"
#include "input/InputSystem.h"
#include "input/Keyboard.h"
#include "input/Mapping.h"
#include "input/Mouse.h"

#include "scene/CameraComponent.h"
#include "scene/CanvasComponent.h"
#include "scene/ComponentRegistry.h"
#include "scene/MaterialComponent.h"
#include "scene/ParticleSystemComponent.h"
#include "scene/Scene.h"
#include "scene/SceneManager.h"
#include "scene/SceneSerializer.h"
#include "scene/ScriptComponent.h"
#include "scene/SourceComponent.h"

#include "ui/Canvas.h"
#include "ui/UIButton.h"
#include "ui/UICheckbox.h"
#include "ui/UICheckboxGroup.h"
#include "ui/UIEventSystem.h"
#include "ui/UIHorizontalBox.h"
#include "ui/UIImage.h"
#include "ui/UILabel.h"
#include "ui/UIScrollView.h"
#include "ui/UISlider.h"
#include "ui/UITextBox.h"
#include "ui/UIVerticalBox.h"

#include "assets/AssetCooker.h"
#include "assets/AssetImporterRegistry.h"
#include "assets/AssetManager.h"
#include "assets/FontAsset.h"
#include "assets/FontCodec.h"
#include "assets/LuaCodec.h"
#include "assets/MusicCodec.h"
#include "assets/ShaderAsset.h"
#include "assets/ShaderCodec.h"
#include "assets/SoundCodec.h"
#include "assets/TextureAsset.h"
#include "assets/TextureCodec.h"
#include "render/PostProcessPipeline.h"

#include "ImageWebP.h" // format module: self-registers WebP decoder + import rule

#include "core/FileSystem.h"
#include "core/Window.h"

#include "gfx/GfxRenderer.h"
#include "gfx/InstanceDrawer.h"

#include "scene/InstancedSpriteComponent.h"
#include "scene/SpriteComponent.h"
#include "resource/SpriteAtlas.h"

#include "utils/MemoryPoolHandler.h"
#include "utils/FrameMemory.h"
#include "utils/EventSystem.h"
#include "utils/MemoryPoolHandler.h"
#include "utils/Random.h"

#include "scripts/ScriptEngine.h"

#include "DemoCook.h"
#include "DemoScene.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

int bh_main(int argc, char **argv) {

  AssetManager::startUp();
    GfxRenderer::startUp();
  // Batches instanced sprites; owns GPU buffers, so it lives inside the context
  // too (shut down before GfxRenderer / the window below).
  InstanceDrawer::startUp();

  // Engine modules. Order matters: SceneManager clears its scenes at shutDown
  // (returning pooled nodes/components), so it is torn down before the pools,
  // and the AssetManager whose sf::Textures they reference is torn down last.
  MemoryPoolHandler::startUp(4096);
  FrameMemory::startUp(4u * 1024u * 1024u);  // 4 MB arena, rewound each frame
  InputSystem::startUp();
  PhysicsSystem::startUp();
  ComponentRegistry::startUp();
  SceneManager::startUp();

  // Release any pending async-load callbacks (they may hold Lua closures) and
  // tear down the scenes (their ScriptComponents hold Lua handles) while the
  // script engine / Lua state is still alive — ScriptEngine / AssetManager shut
  // down just below.
  AssetManager::instance().cancelAsyncLoads();
  SceneManager::instance().destroyAllScenes();

  UIEventSystem::shutDown();

  ScriptEngine::shutDown();
  AssetManager::shutDown();
  // Shut the scene manager down before the pools: it clears every scene, which
  // returns pooled nodes/components while the pools (and SFML) are still alive.
  SceneManager::shutDown();
  ComponentRegistry::shutDown();

  PhysicsSystem::shutDown();
  InputSystem::shutDown();
  FrameMemory::shutDown();
  MemoryPoolHandler::shutDown();

  postFx.reset();
  InstanceDrawer::shutDown();
  GfxRenderer::shutDown();
  Window::shutDown();

  return 0;
}

*/