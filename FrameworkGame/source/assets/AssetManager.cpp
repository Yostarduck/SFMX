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

SPtr<IAsset>
AssetManager::reload(const UUID& id) {
  // Drop the cached copy so load() re-decodes from the current file (or, in debug
  // raw-script mode, re-reads the source). The catalog entry is untouched.
  unload(id);
  return load(id);
}

// ---------------------------------------------------------------------------------
// Async loading
// ---------------------------------------------------------------------------------

SPtr<IAsset>
AssetManager::loadAsync(const UUID& id, Function<void(SPtr<IAsset>)> onLoaded) {
  // Already decoded and cached: defer the callback to the pump so its timing matches
  // the in-flight case (always fired next finalize, on the main thread).
  const auto cached = m_cache.find(id);
  if (cached != m_cache.end()) {
    if (onLoaded) {
      m_readyCallbacks.push_back(ReadyCallback{cached->second, std::move(onLoaded)});
    }
    return cached->second;
  }

  // Already in flight: attach this callback to the existing job, do not enqueue again.
  const auto pending = m_inflight.find(id);
  if (pending != m_inflight.end()) {
    if (onLoaded) {
      pending->second.callbacks.push_back(std::move(onLoaded));
    }
    return pending->second.asset;
  }

  const auto entry = m_catalog.find(id);
  if (entry == m_catalog.end()) {
    std::cerr << "AssetManager::loadAsync: unknown asset " << id.toString()
              << " (not mounted)" << std::endl;
    if (onLoaded) {
      onLoaded(nullptr);  // hard failure, nothing to decode — report immediately
    }
    return nullptr;
  }

  const IAssetCodec* codec = m_codecs.find(entry->second.metadata.assetType);
  if (nullptr == codec) {
    std::cerr << "AssetManager::loadAsync: no codec for assetType "
              << entry->second.metadata.assetType.toString() << " (asset "
              << id.toString() << ")" << std::endl;
    if (onLoaded) {
      onLoaded(nullptr);
    }
    return nullptr;
  }

  SPtr<IAsset> asset = codec->create();
  if (nullptr == asset) {
    if (onLoaded) {
      onLoaded(nullptr);
    }
    return nullptr;
  }
  // Stamp the cataloged metadata and mark it in flight before handing it to the worker.
  asset->setMetadata(entry->second.metadata);
  asset->setState(AssetState::kLoading);

  PendingLoad& p = m_inflight[id];
  p.asset = asset;
  if (onLoaded) {
    p.callbacks.push_back(std::move(onLoaded));
  }

  {
    LockGuard<Mutex> lock(m_jobMutex);
    m_jobQueue.push_back(LoadJob{id, asset, entry->second.path});
  }
  m_jobCv.notify_one();

  return asset;
}

void
AssetManager::workerLoop() {
  for (;;) {
    LoadJob job;
    {
      std::unique_lock<Mutex> lock(m_jobMutex);
      m_jobCv.wait(lock, [this] {
        return m_workerStop.load() || !m_jobQueue.empty();
      });
      if (m_workerStop.load() && m_jobQueue.empty()) {
        return;
      }
      job = std::move(m_jobQueue.front());
      m_jobQueue.pop_front();
    }

    // IO + CPU decode off the main thread. MUST NOT touch pools, m_cache, m_catalog,
    // or any GL resource — only the job's own asset object (which no other thread
    // touches until we hand it back via the done queue).
    SPtr<DataStream> stream = FileSystem::openFile(job.path, AccessMode::kRead);
    AssetFileReader reader;
    if (nullptr != stream && reader.open(stream)) {
      job.asset->decodeCPU(reader);  // sets kLoading (cpu ok) or kFailed
      reader.close();
    }
    else {
      job.asset->setState(AssetState::kFailed);
    }
    stream.reset();  // release the file handle (Windows locks open files)

    {
      LockGuard<Mutex> lock(m_doneMutex);
      m_doneQueue.push_back(job.id);
    }
  }
}

void
AssetManager::finalize() {
  // Drain everything the worker finished since the last pump.
  for (;;) {
    UUID id;
    {
      LockGuard<Mutex> lock(m_doneMutex);
      if (m_doneQueue.empty()) {
        break;  // idle path: one lock, no allocation
      }
      id = m_doneQueue.front();
      m_doneQueue.pop_front();
    }
    finalizeCompleted(id);
  }

  // Fire cache-hit callbacks deferred by loadAsync. Swap out first so a callback that
  // itself calls loadAsync (re-entrancy) does not mutate the vector we iterate.
  if (!m_readyCallbacks.empty()) {
    Vector<ReadyCallback> ready;
    ready.swap(m_readyCallbacks);
    for (ReadyCallback& r : ready) {
      if (r.callback) {
        r.callback(r.asset);
      }
    }
  }
}

void
AssetManager::finalizeCompleted(const UUID& id) {
  const auto it = m_inflight.find(id);
  if (it == m_inflight.end()) {
    return;  // shut down mid-flight, or already handled
  }

  SPtr<IAsset> asset = it->second.asset;

  // Main-thread phase 2 (GPU upload etc.). Byte/PCM assets already reached kLoaded on
  // the worker, so their finalize() is a no-op; a failed CPU decode skips it.
  if (asset->state() != AssetState::kFailed) {
    asset->finalize();
  }

  if (AssetState::kLoaded == asset->state()) {
    m_cache.emplace(id, asset);
  }

  // Move the callbacks out and drop the in-flight entry before firing them: a callback
  // may re-enter loadAsync for this same id (now a clean cache hit / fresh load).
  Vector<Function<void(SPtr<IAsset>)>> callbacks = std::move(it->second.callbacks);
  m_inflight.erase(it);

  for (Function<void(SPtr<IAsset>)>& cb : callbacks) {
    if (cb) {
      cb(asset);  // caller inspects asset->state() for success/failure
    }
  }
}

#if USING(SFMX_DEBUG_MODE)
void
AssetManager::setRawScriptMode(bool enabled, StringView sourceDir) {
  m_rawScripts   = enabled;
  m_rawScriptDir = String(sourceDir);
}
#endif

void
AssetManager::onStartUp() {
  // Spawn the async-load worker; it idle-waits on the job CV until the first loadAsync.
  m_workerStop.store(false);
  m_worker = Thread(&AssetManager::workerLoop, this);
}

void
AssetManager::cancelAsyncLoads() {
  // Stop and join the worker first, so it is no longer touching any asset afterwards.
  m_workerStop.store(true);
  m_jobCv.notify_all();
  if (m_worker.joinable()) {
    m_worker.join();
  }

  // Drop still-in-flight / completed-but-unfinalized loads and their callbacks. The
  // callbacks may hold Lua closures, so releasing them here (while the caller
  // guarantees Lua is alive) avoids destroying them after the script engine is gone.
  {
    LockGuard<Mutex> lock(m_jobMutex);
    m_jobQueue.clear();
  }
  {
    LockGuard<Mutex> lock(m_doneMutex);
    m_doneQueue.clear();
  }
  m_inflight.clear();
  m_readyCallbacks.clear();
}

void
AssetManager::onShutDown() {
  // Join the worker and release pending callbacks (idempotent if already cancelled).
  cancelAsyncLoads();

  // Destroy cached assets (e.g. sf::Texture) while SFML is still alive.
  m_cache.clear();
  m_catalog.clear();
}

} // namespace sfmx
