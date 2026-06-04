#pragma once

#include "core/platform/Prerequisites.h"

namespace sfmx
{

/**
 * @brief Stable, unique identifier for a SceneNode within one Scene.
 *
 * Allocated by the owning Scene from a monotonic counter. Prefer storing a
 * NodeId over a raw SceneNode* whenever a reference must survive the node's
 * lifetime (handles resolve to nullptr once the node is destroyed, pointers
 * dangle).
 */
using NodeId = uint64;

/** @brief Reserved id meaning "no node" / invalid handle. Never allocated. */
constexpr NodeId kInvalidNodeId = 0;

/** @brief Capacity, including the null terminator, of a node's inline name. */
constexpr size_t kMaxNameLength = 32;

/** @brief Runtime identifier for a concrete Component type (see Component.h). */
using ComponentTypeId = uint32;

}
