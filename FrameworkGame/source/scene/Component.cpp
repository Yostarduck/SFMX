#include "scene/Component.h"

// Component (and the ComponentT<> CRTP helper) are header-only: their members
// are defined inline in scene/Component.h. This translation unit exists only so
// the header is compiled on its own and kept self-contained.
