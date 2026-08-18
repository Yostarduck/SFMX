#include "gfx/GfxRenderer.h"

namespace sfmx
{

void
GfxRenderer::onStartUp() {
  // Construction is cheap and context-free on purpose: the renderer resolves its
  // OpenGL entry points on its first draw, when a render target is guaranteed to
  // be active. On Win32 they cannot be resolved any earlier.
  m_quadRenderer = MakeUnique<gfx::InstancedQuadRenderer>();
}

void
GfxRenderer::onShutDown() {
  m_quadRenderer.reset();
}

bool
GfxRenderer::hasQuadRenderer() {
  return isStarted() && nullptr != instance().quadRenderer();
}

} // namespace sfmx
