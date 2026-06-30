#pragma once

namespace demo {

// Cook the demo scene to demo::kSceneFile: build it in code and serialize it,
// then return a process exit code (0 on success). Windowless, but uses SFML's
// implicit GL context (texture decode + atlas frame detection). Requires the
// media to be cooked first (textures resolve by UUID).
int
cookScene();

} // namespace demo
