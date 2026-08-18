#pragma once

#include "gfx/InstancedQuadRenderer.h"

#include "core/platform/Prerequisites.h"
#include "utils/Module.h"

namespace sfmx
{

/**
 * @brief Owns the renderers that are shared by every drawable in the engine.
 *
 * Right now that is a single @ref gfx::InstancedQuadRenderer, which holds a
 * compiled shader program. The only reason this module exists is to give that
 * program a deterministic destruction point while the OpenGL context is still
 * alive: a function-local static would be torn down at exit in an order-dependent
 * race against SFML's own globals.
 *
 * A @ref Module (singleton). Start it right after @ref Window and shut it down
 * right before, so the context outlives everything it owns. Forgetting to start it
 * is not fatal -- drawables that can fall back to a plain SFML path will do so.
 */
class GfxRenderer : public Module<GfxRenderer>
{
 public:
  GfxRenderer() = default;
  ~GfxRenderer() override = default;

  /** @brief The shared instanced quad renderer, or null if start-up failed. */
  NODISCARD FORCEINLINE gfx::InstancedQuadRenderer*
  quadRenderer() const { return m_quadRenderer.get(); }

  /**
   * @brief Whether a caller can route a batch through @ref quadRenderer.
   *
   * This only says the module is up; whether the driver actually supports
   * instancing is not known until the first draw, so callers must still handle
   * @c gfx::InstancedQuadRenderer::draw returning false.
   */
  NODISCARD static bool
  hasQuadRenderer();

 protected:
  void onStartUp() override;
  void onShutDown() override;

 private:
  UniquePtr<gfx::InstancedQuadRenderer> m_quadRenderer;
};

} // namespace sfmx
