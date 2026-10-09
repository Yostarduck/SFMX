#include "ui/UISerializer.h"

#include <cstdio>

#include "assets/AssetFile.h"
#include "core/DataStream.h"
#include "core/DataStreamTypes.h"  // UUID stream operators (raw 16 bytes)
#include "core/FileSystem.h"
#include "core/MemoryDataStream.h"
#include "ui/UIWidget.h"
#include "ui/UIWidgetRegistry.h"

namespace sfmx
{

namespace
{

/** @brief The assetType tag stamped on a UI document `.sfmxasset`. */
const UUID&
uiDocumentAssetType() {
  static const UUID id = UUID::createFromName("UIDocument");
  return id;
}

/**
 * @brief Write @p widget's record and its subtree, depth-first.
 *
 * Record = [type UUID][payload byte size][child count][payload] + children.
 * The size prefix lets a reader skip an unknown type; the child count keeps
 * the stream aligned either way (same trick SceneSerializer uses for
 * components).
 */
bool
writeRecord(UIWidget* widget, DataStream& out) {
  MemoryDataStream payload;
  widget->onSerialize(payload);

  out << widget->getTypeId();
  out << static_cast<uint64>(payload.size());

  const Vector<UIWidget*>& children = widget->getChildren();
  out << static_cast<uint32>(children.size());

  if (payload.size() > 0) {
    out.write(payload.data(), payload.size());
  }
  for (UIWidget* child : children) {
    if (nullptr == child || !writeRecord(child, out)) {
      return false;
    }
  }
  return true;
}

/** @brief Consume one record (payload + child subtrees) without creating. */
void
skipRecord(DataStream& in) {
  UUID typeId;
  uint64 size = 0;
  uint32 childCount = 0;
  in >> typeId >> size >> childCount;

  Vector<uint8> bytes(static_cast<size_t>(size));
  if (size > 0) {
    in.read(bytes.data(), static_cast<size_t>(size));
  }
  for (uint32 i = 0; i < childCount; ++i) {
    skipRecord(in);
  }
}

/** @brief Rebuild one record; nullptr = unknown type (record fully consumed). */
UIWidget*
readRecord(DataStream& in) {
  UUID typeId;
  uint64 size = 0;
  uint32 childCount = 0;
  in >> typeId;
  in >> size;
  in >> childCount;

  Vector<uint8> bytes(static_cast<size_t>(size));
  if (size > 0) {
    in.read(bytes.data(), static_cast<size_t>(size));
  }

  UIWidget* widget = UIWidgetRegistry::instance().create(typeId, {});
  if (nullptr == widget) {
    // Unknown type: its payload bytes are already consumed; its children
    // would have no parent to attach to, so drop them from the stream too.
    for (uint32 i = 0; i < childCount; ++i) {
      skipRecord(in);
    }
    return nullptr;
  }

  // Feed only this widget's bytes so it cannot read past its slice.
  MemoryDataStream slice(bytes.data(), bytes.size());
  widget->onDeserialize(slice);

  for (uint32 i = 0; i < childCount; ++i) {
    if (UIWidget* child = readRecord(in)) {
      widget->addChild(child);
    }
    // else: the unknown child consumed itself — remaining children still attach.
  }
  return widget;
}

} // namespace

bool
UISerializer::serialize(const Vector<UIWidget*>& roots, DataStream& out) {
  out << static_cast<uint32>(roots.size());
  for (UIWidget* root : roots) {
    if (nullptr == root || !writeRecord(root, out)) {
      return false;
    }
  }
  return true;
}

bool
UISerializer::deserialize(Vector<UIWidget*>& outRoots, DataStream& in) {
  outRoots.clear();
  if (!UIWidgetRegistry::isStarted()) {
    return false;
  }

  uint32 rootCount = 0;
  in >> rootCount;
  for (uint32 i = 0; i < rootCount; ++i) {
    if (UIWidget* root = readRecord(in)) {
      outRoots.push_back(root);
    }
  }
  return true;
}

bool
UISerializer::saveToFile(const Vector<UIWidget*>& roots,
                         const FileSystemPath& path) {
  MemoryDataStream blob;
  if (!serialize(roots, blob)) {
    return false;
  }

  AssetFileWriter writer;
  AssetMetadata meta;
  meta.assetType = uiDocumentAssetType();
  std::snprintf(meta.name, sizeof(meta.name), "%s", "UIDocument");
  writer.setMetadata(meta);
  // Structured widget records (very compressible): raw chunk, LZ4-compressed
  // on write; the reader inflates it transparently on load. (Falls back to
  // uncompressed automatically if it would not shrink — see AssetFileWriter.)
  writer.addChunk(blob.data(), blob.size(), ChunkFormat::kRaw,
                  ChunkCompression::kLz4);

  SPtr<DataStream> out =
      FileSystem::createAndOpenFile(FileSystem::resolve(path));
  if (nullptr == out) {
    return false;
  }
  const bool ok = writer.writeTo(*out);
  out->close();
  return ok;
}

bool
UISerializer::loadFromFile(Vector<UIWidget*>& outRoots,
                           const FileSystemPath& path) {
  SPtr<DataStream> in =
      FileSystem::openFile(FileSystem::resolve(path), AccessMode::kRead);
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
  return deserialize(outRoots, blob);
}

} // namespace sfmx
