#include "scene/SceneSerializer.h"

#include <cstdio>

#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>

#include "core/DataStream.h"
#include "core/DataStreamTypes.h"   // operator<< / >> for UUID
#include "core/FileSystem.h"
#include "core/MemoryDataStream.h"
#include "assets/AssetFile.h"
#include "scene/ComponentRegistry.h"
#include "scene/Scene.h"
#include "scene/SceneNode.h"
#include "scene/Transform.h"

namespace sfmx
{

namespace
{

/** @brief Scene blob layout version; bump on format changes. */
constexpr uint32 kSceneFormatVersion = 1;

/** @brief The assetType tag stamped on a scene `.sfmxasset`. */
const UUID&
sceneAssetType() {
  static const UUID id = UUID::createFromName("Scene");
  return id;
}

// Pre-order (parent-before-child) gather of every descendant of @p node.
void
gatherDescendants(const SceneNode* node, Vector<const SceneNode*>& out) {
  for (const SceneNode* child = node->getFirstChild();
       nullptr != child;
       child = child->getNextSibling()) {
    out.push_back(child);
    gatherDescendants(child, out);
  }
}

void
writeNode(DataStream& out, const SceneNode& node) {
  out << node.getId();
  out << node.getParent()->getId();  // gathered nodes always have a parent
  out.writeString(node.getName());
  out << static_cast<uint8>(node.isEnabled() ? 1 : 0);
  out << static_cast<uint8>(node.isVisible() ? 1 : 0);

  const Transform& t = node.transform();
  const sf::Vector2f pos = t.getPosition();
  const sf::Vector2f scale = t.getScale();
  out << pos.x << pos.y;
  out << t.getRotation().asRadians();
  out << scale.x << scale.y;

  uint32 componentCount = 0;
  for (const Component* c = node.getFirstComponent();
       nullptr != c;
       c = c->getNextComponent()) {
    ++componentCount;
  }
  out << componentCount;

  for (const Component* c = node.getFirstComponent();
       nullptr != c;
       c = c->getNextComponent()) {
    // Serialize each component into its own buffer so we can prefix the byte
    // size — that lets the reader skip a component whose type is unknown.
    MemoryDataStream tmp;
    c->onSerialize(tmp);
    out << c->getTypeId();
    const uint64 size = static_cast<uint64>(tmp.size());
    out << size;
    if (size > 0) {
      out.write(tmp.data(), static_cast<size_t>(size));
    }
  }
}

} // namespace

bool
SceneSerializer::serialize(const Scene& scene, DataStream& out) {
  if (!out.isWriteable()) {
    return false;
  }

  Vector<const SceneNode*> nodes;
  gatherDescendants(scene.getRoot(), nodes);

  out << kSceneFormatVersion;
  out << scene.getRoot()->getId();                 // rootId
  out << static_cast<uint64>(nodes.size());        // nodeCount (excludes root)

  for (const SceneNode* node : nodes) {
    writeNode(out, *node);
  }
  return true;
}

bool
SceneSerializer::deserialize(Scene& scene, DataStream& in) {
  if (!in.isReadable()) {
    return false;
  }

  uint32 version = 0;
  in >> version;
  if (version != kSceneFormatVersion) {
    return false;  // v1: exact match; migrate older versions here later
  }

  NodeId rootId = kInvalidNodeId;
  uint64 nodeCount = 0;
  in >> rootId;
  in >> nodeCount;

  scene.clear();

  UnorderedMap<NodeId, SceneNode*> idMap;
  idMap[rootId] = scene.getRoot();

  for (uint64 i = 0; i < nodeCount; ++i) {
    NodeId id = kInvalidNodeId;
    NodeId parentId = kInvalidNodeId;
    in >> id;
    in >> parentId;
    const String name = in.readString();

    uint8 enabled = 1;
    uint8 visible = 1;
    in >> enabled;
    in >> visible;

    float px = 0.f, py = 0.f, rot = 0.f, sx = 1.f, sy = 1.f;
    in >> px >> py >> rot >> sx >> sy;

    uint32 componentCount = 0;
    in >> componentCount;

    // Parent was emitted before this node (pre-order), so it is already mapped;
    // fall back to the root if a referenced parent is somehow missing.
    const auto parentIt = idMap.find(parentId);
    SceneNode* parent = parentIt != idMap.end() ? parentIt->second
                                                 : scene.getRoot();

    SceneNode* node = scene.createNode(name, parent);
    if (nullptr == node) {
      return false;  // node pool exhausted
    }
    idMap[id] = node;

    node->setEnabled(enabled != 0);
    node->setVisible(visible != 0);
    node->transform().setPosition({px, py});
    node->transform().setRotation(sf::radians(rot));
    node->transform().setScale({sx, sy});

    for (uint32 c = 0; c < componentCount; ++c) {
      UUID typeId;
      uint64 size = 0;
      in >> typeId;
      in >> size;

      // Always consume the component's bytes, even if we can't create it.
      Vector<uint8> bytes;
      bytes.resize(static_cast<size_t>(size));
      if (size > 0) {
        in.read(bytes.data(), static_cast<size_t>(size));
      }

      Component* component =
          ComponentRegistry::instance().create(typeId, node);
      if (nullptr != component) {
        // Feed only this component's bytes so it cannot read past its slice.
        MemoryDataStream slice(bytes.data(), bytes.size());
        component->onDeserialize(slice);
      }
      // else: unknown component type — bytes already skipped, attach nothing.
    }
  }
  return true;
}

bool
SceneSerializer::saveToFile(const Scene& scene, const FileSystemPath& path) {
  MemoryDataStream blob;
  if (!serialize(scene, blob)) {
    return false;
  }

  AssetFileWriter writer;
  AssetMetadata meta;
  meta.assetType = sceneAssetType();
  std::snprintf(meta.name, sizeof(meta.name), "%s", scene.getName());
  writer.setMetadata(meta);
  writer.addChunk(blob.data(), blob.size(), ChunkFormat::kRaw);

  SPtr<DataStream> out = FileSystem::createAndOpenFile(FileSystem::resolve(path));
  if (nullptr == out) {
    return false;
  }
  const bool ok = writer.writeTo(*out);
  out->close();
  return ok;
}

bool
SceneSerializer::loadFromFile(Scene& scene, const FileSystemPath& path) {
  SPtr<DataStream> in = FileSystem::openFile(FileSystem::resolve(path), AccessMode::kRead);
  if (nullptr == in) {
    return false;
  }

  AssetFileReader reader;
  if (!reader.open(in) || reader.chunkCount() == 0) {
    return false;
  }

  Vector<uint8> bytes;
  const bool read = reader.readChunk(0, bytes);
  reader.close();
  in.reset();
  if (!read) {
    return false;
  }

  MemoryDataStream blob(bytes.data(), bytes.size());
  return deserialize(scene, blob);
}

} // namespace sfmx
