#include "core/MemoryPool.h"

// MemoryPool<T> is a class template: its member definitions must be visible at
// the point of instantiation, so they live inline in core/MemoryPool.h. This
// translation unit intentionally holds no out-of-line definitions; it exists
// only so the header is compiled on its own and kept self-contained.