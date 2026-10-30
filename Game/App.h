#pragma once

#include "core/platform/Prerequisites.h"

#include "config/IniFile.h"

namespace sfmx
{

/**
 * @brief Minimal application lifecycle contract shared by every executable
 *        front-end.
 *
 * An @ref App is created on the stack by `main`, driven through a fixed
 * sequence, and destroyed afterwards:
 *
 * @code
 * MyApp app;                    // any App implementation, e.g. sfmx::Game
 * app.initialize(argc, argv);   // parse args, load content, build the world
 * if (!app.shouldExit()) {      // false only when initialize() handled a
 *   app.run();                  // one-shot command such as `--cook`
 * }
 * app.shutdown();               // release everything initialize() acquired
 * return app.exitCode();        // 0 on success
 * @endcode
 *
 * The individual frame steps (@ref pollEvents, @ref update, @ref render) are
 * public so a host can drive frames itself (tests, tools, an editor) instead
 * of calling @ref run. @ref run is only the default loop that calls those
 * three in order until the window closes.
 *
 * @ref shouldExit and @ref exitCode are independent: a command such as
 * @c --cook sets @ref shouldExit without implying an error, while
 * @ref exitCode carries the process status the shell or build script sees.
 *
 * Implementations own their engine resources and are not copyable. @ref
 * shutdown must be safe to call after an @ref initialize that only handled a
 * command (no engine modules were started), and must leave no module
 * registered.
 */
class App {
 public:
  App() = default;
  virtual ~App() = default;

  /**
   * @brief Parse arguments, handle a one-shot command, and start the engine.
   *
   * After returning, callers must check @ref shouldExit: when it is true the
   * command has already run and @ref run must not be called. Implementations
   * set @ref exitCode to the command's status. Throws on unrecoverable
   * start-up errors; the front-end is expected to catch and report them.
   *
   * @param argc Argument count as passed to `main`.
   * @param argv Argument vector as passed to `main`; @c argv[0] is the program
   *             name and is not interpreted as an option.
   */
  virtual void
  initialize(int argc, char **argv) = 0;

  /**
   * @brief Drain the platform event queue for the current frame.
   *
   * Implementations route events to the UI first (so a focused editor can
   * consume typing) and then to the input system. Called once per frame by
   * @ref run before @ref update.
   */
  virtual void
  pollEvents() = 0;

  /**
   * @brief Advance one frame of simulation and UI.
   *
   * @param deltaTime Seconds elapsed since the previous frame.
   */
  virtual void
  update(float deltaTime) = 0;

  /**
   * @brief Draw the current frame and present it.
   *
   * Called once per frame after @ref update. Implementations are expected to
   * reclaim per-frame scratch memory (and any transient GPU state) before
   * returning.
   */
  virtual void
  render() = 0;

  /**
   * @brief Run the default frame loop until the window closes.
   *
   * Repeatedly calls @ref pollEvents, @ref update (with the measured delta)
   * and @ref render. Blocking; must not be called when @ref shouldExit is true.
   */
  virtual void
  run() = 0;

  /**
   * @brief Release everything @ref initialize acquired.
   *
   * Implementations tear engine modules down in reverse start-up order. When
   * @ref initialize only handled a command (nothing was started) this is a
   * no-op, which makes the standard `initialize`/@ref shutdown sequence safe
   * for every mode. Idempotent.
   */
  virtual void
  shutdown() = 0;

  /** @brief True when initialize() handled a command and no frame loop should run. */
  NODISCARD bool
  shouldExit() const { return m_shouldExit; }

  /** @brief Process exit status; 0 means success. */
  NODISCARD int
  exitCode() const { return m_exitCode; }

 protected:
  /** @brief Shared INI storage for the implementation (`Engine.ini` + `Game.ini`). */
  IniFile m_config;

  /** @brief Set by initialize() when a one-shot command handled the process. */
  bool m_shouldExit = false;

  /** @brief Process status reported by `main()`; 0 on success. */
  int m_exitCode = 0;
};

} // namespace sfmx
