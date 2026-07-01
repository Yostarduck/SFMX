#include "assets/AssetManager.h"

#include <iostream>

#include "assets/AssetFile.h"
#include "assets/IAssetCodec.h"
#include "core/DataStream.h"
#include "core/FileSystem.h"

namespace sfmx
{

void
AssetManager::registerCodec(SPtr<IAssetCodec> codec) {
  m_codecs.registerCodec(std::move(codec));
}

size_t
AssetManager::mount(const FileSystemPath& directoryArg) {
  // Relative dirs resolve under the content root (exe dir at runtime); absolute
  // dirs (e.g. tests' temp dirs) pass through unchanged.
  const FileSystemPath directory = FileSystem::resolve(directoryArg);
  if (!FileSystem::isDirectory(directory)) {
    std::cerr << "AssetManager::mount: not a directory: " << directory.string()
              << std::endl;
    return 0;
  }

  size_t cataloged = 0;
  FileSystem::forEachFileChildRecursive(
      directory, [&](const FileSystemPath& path) {
        if (!FileSystem::isFile(path) || path.extension() != ".sfmxasset") {
          return;
        }

        SPtr<DataStream> stream = FileSystem::openFile(path, AccessMode::kRead);
        AssetFileReader reader;
        if (nullptr == stream || !reader.open(stream)) {
          //TODO: Make a log that notifies for warnings 
          return;  // not a valid .sfmxasset; skip it
        }

        const UUID& id = reader.metadata().uuid;
        if (m_catalog.find(id) != m_catalog.end()) {
          std::cerr << "AssetManager::mount: duplicate asset id " << id.toString()
                    << " (" << path.string() << "); keeping the first" << std::endl;
          return;
        }

        m_catalog.emplace(id, CatalogEntry{path, reader.metadata()});
        ++cataloged;
      });

  return cataloged;
}

SPtr<IAsset>
AssetManager::load(const UUID& id) {
  // Cache hit: already decoded.
  const auto cached = m_cache.find(id);
  if (cached != m_cache.end()) {
    return cached->second;
  }

  // Reference cycle: this id is already being loaded further up the call stack.
  if (m_loading.find(id) != m_loading.end()) {
    //TODO: Make a log that notifies for warnings. Akthough this might not be a warning and a simply log instead
    return nullptr;
  }

  const auto entry = m_catalog.find(id);
  if (entry == m_catalog.end()) {
    std::cerr << "AssetManager::load: unknown asset " << id.toString()
              << " (not mounted)" << std::endl;
    return nullptr;
  }

  m_loading.insert(id);

  SPtr<DataStream> stream = FileSystem::openFile(entry->second.path,
                                                 AccessMode::kRead);
  AssetFileReader reader;
  SPtr<IAsset> asset = nullptr;
  if (nullptr != stream && reader.open(stream)) {
    // Resolve dependencies first so they are cached before this asset (or the
    // scene built from it) tries to resolve them. Best-effort: a missing dep is
    // logged by the recursive call but does not abort this load.
    for (const UUID& ref : reader.references()) {
      // We want only the side effect (the dep lands in m_cache), not the handle,
      // so the static_cast<void> deliberately discards load()'s NODISCARD result
      // it silences the "discarding [[nodiscard]]" warning and marks intent.
      static_cast<void>(load(ref));
    }
    asset = m_codecs.decode(reader);  // dispatches on metadata().assetType
    reader.close();
  } else {
    std::cerr << "AssetManager::load: cannot open " << entry->second.path.string()
              << std::endl;
  }
  stream.reset();  // release the file handle (Windows locks open files)

  m_loading.erase(id);

  if (nullptr != asset) {
    m_cache.emplace(id, asset);
  }
  return asset;
}

SPtr<IAsset>
AssetManager::get(const UUID& id) const {
  const auto it = m_cache.find(id);
  return it != m_cache.end() ? it->second : nullptr;
}

bool
AssetManager::isCataloged(const UUID& id) const {
  return m_catalog.find(id) != m_catalog.end();
}

bool
AssetManager::isLoaded(const UUID& id) const {
  return m_cache.find(id) != m_cache.end();
}

const AssetMetadata*
AssetManager::metadataOf(const UUID& id) const {
  const auto it = m_catalog.find(id);
  return it != m_catalog.end() ? &it->second.metadata : nullptr;
}

void
AssetManager::unload(const UUID& id) {
  m_cache.erase(id);
}

void
AssetManager::unloadAll() {
  m_cache.clear();
}

void
AssetManager::onShutDown() {
  // Destroy cached assets (e.g. sf::Texture) while SFML is still alive.
  m_cache.clear();
  m_catalog.clear();
}

} // namespace sfmx
