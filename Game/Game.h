#pragma once

#include <array>

#include <SFML/System/Clock.hpp>

#include "App.h"
#include "core/ArgumentParser.h"
#include "render/PostProcessPipeline.h"
#include "utils/EventSystem.h"

namespace sfmx
{

class Scene;
class SceneNode;
class ParticleSystemComponent;
class UILabel;

/**
 * @brief The SFMX game application: command dispatch, engine bootstrap, frame
 *        loop, and teardown.
 *
 * `main()` only constructs a @ref Game, forwards `argc`/`argv`, and calls the
 * @ref App lifecycle. Everything else lives here so the entry point stays a
 * few lines and the start-up order has exactly one owner.
 *
 * @par Modes
 * @ref initialize has two modes:
 *  - **Runtime** (no command flag): applies the content-root override, loads
 *    @c config/Engine.ini and @c config/Game.ini, then runs the ordered
 *    start-up phases. `main` then calls @ref run unless @ref shouldExit is set.
 *  - **Offline** (@c --cook, @c --cook-scene): runs the one-shot command
 *    without creating a window, sets @ref shouldExit, and reports success
 *    through @ref exitCode. @ref shutdown is a no-op in this mode.
 *
 * @par Start-up order
 * The runtime phases depend on this exact order:
 *  - @ref initWindow — window first; GL-backed modules initialize inside its
 *    context.
 *  - @ref initModules — pools, frame arena, input, physics, component
 *    registry, scene manager, then the demo pools/components.
 *  - @ref initAssets — asset manager, codecs, WebP module, script mode, and
 *    the @c assets mount (needs the window for texture uploads).
 *  - @ref initScriptingAndUI — @c UIManager before @c ScriptEngine so the
 *    Lua `UI` global is available when scripts start.
 *  - @ref initScene — load the cooked scene (or build it in code) and wire
 *    the runtime behavior the serialized data does not carry.
 *  - @ref initPostProcessing — own the GL render targets and the CRT pass.
 *  - @ref initInput — create the active mapping and wire the UI actions.
 *  - @ref initUI — build the HUD widget tree and subscribe its handlers.
 *  - @ref initDemoContent — add the demo scripts, particles, and stress grids.
 *
 * @par Frame order
 * @ref run calls @ref pollEvents, measures the frame delta, then calls
 * @ref update and @ref render. @ref update advances input, asset finalization,
 * UI, and scenes. @ref render draws the scene through the optional post chain,
 * then the UI, presents the frame, and rewinds the frame arena.
 *
 * @par Ownership
 * The scene and its nodes/components are owned by @c SceneManager, so
 * @ref m_scene, @ref m_particleSystem, and the stress-node handles are
 * non-owning and become dangling after @ref shutdown. @ref m_postFx owns GL
 * resources and is destroyed before the window. Destroy the @ref Game after
 * calling @ref shutdown.
 */
class Game final : public App {
 public:
  Game() = default;
  ~Game() override = default;

  /**
   * @brief Parse arguments, dispatch a command, or start the game.
   *
   * Recognizes @c --cook [src] [out], @c --cook-scene, and
   * @c --content-dir \<path\>. Command flags run headless and return with
   * @ref shouldExit set; any other argument set starts the full runtime.
   *
   * The content root is resolved before config/content loads in this order:
   * @c --content-dir, then the @c SFMX_CONTENT_ROOT environment variable, then
   * the executable directory. Config paths are read under that root.
   *
   * @param argc Argument count as passed to `main`.
   * @param argv Argument vector as passed to `main`; @c argv[0] is skipped.
   *
   * @throws std::runtime_error If a module is used out of its lifecycle order
   *         or a required asset/window cannot be created.
   */
  void
  initialize(int argc, char **argv) override;

  /**
   * @brief Drain window events for the current frame.
   *
   * Snapshots input device state first, then routes each event to the UI; if
   * the UI consumes it (focused text editor, navigation, etc.) the game input
   * layer does not see that event. Otherwise the event goes to
   * @c InputSystem. A window-close event closes the window, which ends
   * @ref run on the next iteration.
   */
  void
  pollEvents() override;

  /**
   * @brief Advance one frame of input, UI, and scene simulation.
   *
   * Records the delta in the HUD rolling window, updates the HUD text, polls
   * input and dev hotkeys, finalizes completed background asset loads (so
   * scripts see fresh assets this same frame), updates the UI, then updates
   * every active scene. Finally accumulates the total run time used by the
   * post-processing shaders.
   *
   * @param deltaTime Seconds elapsed since the previous frame.
   */
  void
  update(float deltaTime) override;

  /**
   * @brief Render and present one frame.
   *
   * Clears the window, draws the scene through @ref m_postFx when present (or
   * directly otherwise), draws the screen-space UI afterwards so the HUD stays
   * crisp, presents, and reclaims the frame arena.
   */
  void
  render() override;

  /**
   * @brief Run the default frame loop until the window closes.
   *
   * Calls @ref pollEvents, samples the clock, then @ref update and
   * @ref render. Blocking; must not be called when @ref shouldExit is true.
   */
  void
  run() override;

  /**
   * @brief Tear down the engine in reverse start-up order.
   *
   * No-op when @ref initialize handled a cook command (no modules were
   * started). Disconnects UI callbacks, cancels async loads, destroys scenes
   * while scripts/Lua are alive, shuts down UI/scripts/assets/scenes/registry,
   * physics/input/frame memory/pools, then releases the GL objects and the
   * window. Resets @ref m_initialized, so it is safe to call twice.
   */
  void
  shutdown() override;

 private:
  // -- Command modes --------------------------------------------------------
  /**
   * @brief Cook media into @c .sfmxasset containers and report success.
   *
   * Uses @c "Game/resources" and @c "Game/assets" when the flag carries no
   * values; otherwise the first value is the source directory and the second
   * is the output directory. Starts the importer registry, registers the
   * built-in rules plus WebP, cooks recursively, and shuts the registry down.
   *
   * @return False when the source directory is missing or nothing could be
   *         cooked; true when at least one file was wrapped.
   */
  bool
  cook();

  /**
   * @brief Cook the demo scene to @c demo::kSceneFile.
   *
   * Delegates to @c demo::cookScene, which builds the scene in code and
   * serializes it. The media must already be cooked because textures resolve
   * by UUID.
   *
   * @return True when the scene was written (exit code 0 from the cook).
   */
  bool
  cookGame();

  // -- Startup phases (ordered, mirror Game::shutdown) -----------------------
  /**
   * @brief Apply @c --content-dir or @c SFMX_CONTENT_ROOT over the default
   *        executable-relative content root.
   *
   * Must run before any config or content path is resolved. An empty override
   * leaves the default in place.
   */
  void
  applyContentRootOverride();

  /**
   * @brief Create the window and the GL modules that depend on its context.
   *
   * Starts @c Window with the configured title/size and VSync, then
   * @c GfxRenderer and @c InstanceDrawer. Must be the first runtime phase.
   */
  void
  initWindow();

  /**
   * @brief Start the non-GL engine modules and register demo types.
   *
   * Order is significant: pools and the frame arena first, then input,
   * physics, the component registry, and the scene manager, followed by the
   * demo pools/components. Teardown observes the reverse order.
   */
  void
  initModules();

  /**
   * @brief Start the asset manager, register codecs, and mount @c assets.
   *
   * Requires the window for texture uploads. Registers the texture, shader,
   * Lua, sound, music, and font codecs, the WebP image module, enables raw
   * Lua loading in debug builds, and mounts the cooked asset directory.
   */
  void
  initAssets();

  /**
   * @brief Start the UI manager and the script engine.
   *
   * @c UIManager must come first: its pools are used by widget creation and
   * @c ScriptEngine binds the Lua `UI` global only when the manager is
   * already started.
   */
  void
  initScriptingAndUI();

  /**
   * @brief Load the cooked scene, or build it in code when the file is absent.
   *
   * Resolves @c demo::kSceneFile under the content root, then calls
   * @c demo::wireDemoRuntime for state the serialized scene does not carry
   * (active camera, subscriptions, etc.). Requires the scene manager and the
   * asset manager to be started.
   */
  void
  initScene();

  /**
   * @brief Create the post-processing pipeline and add the CRT pass.
   *
   * The pipeline is constructed in @ref m_postFx so its render textures are
   * destroyed before the window. Requires the window and asset manager.
   */
  void
  initPostProcessing();

  /**
   * @brief Create the default input mapping and wire the UI actions.
   *
   * Builds the @c UI map with Navigate (2D composite over arrows/WASD), Submit
   * (Space/Enter), and Cancel (Escape), activates the mapping, and hands the
   * actions to @c UIManager for keyboard/gamepad navigation.
   */
  void
  initInput();

  /**
   * @brief Build the HUD widget tree and subscribe the shader toggle.
   *
   * Loads the UI font, creates the debug/status labels, upgrades menu, and
   * bottom-right buttons, then wires the post-processing toggle. Requires
   * @ref initScriptingAndUI and @ref initPostProcessing to have run.
   */
  void
  initUI();

  /**
   * @brief Add the demo content that is still authored in code.
   *
   * Creates the GameManager and InputDemo script nodes, the particle emitter
   * with its custom-data material, and the two 45k-sprite stress grids used by
   * the B hotkey. Requires the scene and asset manager to be ready.
   */
  void
  initDemoContent();

  // -- Frame helpers -------------------------------------------------------
  /**
   * @brief Refresh the debug label with FPS, node/draw counts, and toggles.
   *
   * Averaged over @ref kDeltaSamples frames. The text is built in the frame
   * arena instead of a temporary heap string.
   */
  void
  updateHud();

  /**
   * @brief Handle the development hotkeys for the current frame.
   *
   * - @c Esc: close the window.
   * - @c I: print the live particle count.
   * - @c B: flip between the instanced and regular stress grids.
   * - @c V: toggle VSync.
   * - @c F5 (debug raw-script mode): re-decode and re-bind every script.
   */
  void
  handleInput();

  // -- Scene/content state --------------------------------------------------
  /** @brief Active scene; owned by @c SceneManager, not by this object. */
  Scene* m_scene = nullptr;

  /** @brief Demo emitter handle; owned by a scene node. */
  ParticleSystemComponent* m_particleSystem = nullptr;

  /** @brief Instanced stress grid root; owned by the active scene. */
  SceneNode* m_stressInstanced = nullptr;

  /** @brief Regular (per-sprite) stress grid root; owned by the active scene. */
  SceneNode* m_stressRegular = nullptr;

  /** @brief Sprites per stress grid (also the regular-mode draw-call count). */
  size_t m_stressCount = 0;

  /** @brief Which stress grid is visible; toggled with @c B. */
  bool m_showInstanced = true;

  /** @brief Current VSync state; toggled with @c V. */
  bool m_vsyncOn = true;

  // -- UI state -------------------------------------------------------------
  /** @brief HUD label showing FPS/counters; owned by @c UIManager. */
  UILabel* m_debugLabel = nullptr;

  /** @brief Label showing the post-processing on/off state. */
  UILabel* m_shaderLabel = nullptr;

  /** @brief Subscription for the shader-toggle button; reset before UI teardown. */
  HEvent m_toggleShaderHandle;

  // -- Rendering ------------------------------------------------------------
  // Optional because its render textures must be released before the window.
  /** @brief Full-screen pass chain; reset before the window shuts down. */
  Optional<PostProcessPipeline> m_postFx;

  // -- Timing ---------------------------------------------------------------
  /** @brief Frame clock; restarted every iteration of @ref run. */
  sf::Clock m_clock;

  /** @brief Number of samples in the rolling HUD average. */
  static constexpr size_t kDeltaSamples = 100;

  /** @brief Rolling frame-delta window used by @ref updateHud. */
  std::array<float, kDeltaSamples> m_deltas{};

  /** @brief Next slot in @ref m_deltas (wraps at @ref kDeltaSamples). */
  uint32 m_deltaIndex = 0;

  /** @brief Total elapsed seconds, fed to the post-processing shaders. */
  float m_totalTime = 0.f;

  // -- Configuration --------------------------------------------------------
  /** @brief Parsed command line; consulted by @ref initialize and the cooks. */
  ArgumentParser m_argParser;

  /** @brief Window width from @c Engine.ini, used for layout init. */
  uint32 m_windowWidth = 800;

  /** @brief Window height from @c Engine.ini, used for layout init. */
  uint32 m_windowHeight = 600;

  /** @brief True once all runtime phases completed; gates @ref shutdown. */
  bool m_initialized = false;
};

} // namespace sfmx
