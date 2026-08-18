#include "assets/ShaderAsset.h"

#include <algorithm>
#include <cctype>
#include <iostream>

#include "assets/AssetFile.h"
#include "core/FileSystem.h"

namespace sfmx
{

namespace
{

// Copy a chunk's raw bytes into a source string. GLSL is plain text; the chunk is
// stored verbatim, so no decode step is needed here.
FORCEINLINE void
chunkToString(const Vector<uint8>& bytes, String& out) {
  out.assign(reinterpret_cast<const ansichar*>(bytes.data()), bytes.size());
}

// Trim leading/trailing whitespace in place — manifest tokens may carry stray
// spaces or a trailing carriage return on Windows-authored files.
void
trim(String& s) {
  const auto notSpace = [](ansichar c) {
    return 0 == std::isspace(static_cast<unsigned char>(c));
  };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
  s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
}

} // namespace

bool
ShaderAsset::decodeFrom(AssetFileReader& reader) {
  return decodeCPU(reader) && finalize();
}

bool
ShaderAsset::decodeCPU(AssetFileReader& reader) {
  // WORKER thread: read each stage chunk into its source string. No GL here — the
  // compile/link waits for finalize() on the main thread.
  setMetadata(reader.metadata());

  const size_t count = reader.chunkCount();
  if (0 == count) {
    setState(AssetState::kFailed);
    return false;
  }

  // Load-time scratch, never touched in the game loop.
  Vector<uint8> bytes;
  bool any = false;
  for (size_t i = 0; i < count; ++i) {
    const ChunkFormatId& format = reader.chunk(i).format;
    if (!reader.readChunk(i, bytes) || bytes.empty()) {
      continue;
    }
    if (format == ShaderChunk::kVert) {
      chunkToString(bytes, m_vertexSrc);
      any = true;
    }
    else if (format == ShaderChunk::kGeom) {
      chunkToString(bytes, m_geometrySrc);
      any = true;
    }
    else {
      // Default to fragment: a loose `.frag` cooks to a single fragment chunk, and
      // anything not explicitly a vertex/geometry stage is treated as fragment.
      chunkToString(bytes, m_fragmentSrc);
      any = true;
    }
  }

  setState(any ? AssetState::kLoading : AssetState::kFailed);
  return any;
}

bool
ShaderAsset::finalize() {
  // MAIN thread: compile/link the GLSL sources into the GPU shader (needs GL).
  bool ok = false;
  if (sf::Shader::isAvailable()) {
    const bool hasVert = !m_vertexSrc.empty();
    const bool hasFrag = !m_fragmentSrc.empty();
    const bool hasGeom = !m_geometrySrc.empty();

    if (hasVert && hasFrag && hasGeom) {
      ok = sf::Shader::isGeometryAvailable() &&
           m_shader.loadFromMemory(m_vertexSrc, m_geometrySrc, m_fragmentSrc);
    }
    else if (hasVert && hasFrag) {
      ok = m_shader.loadFromMemory(m_vertexSrc, m_fragmentSrc);
    }
    else if (hasFrag) {
      ok = m_shader.loadFromMemory(m_fragmentSrc, sf::Shader::Type::Fragment);
    }
    else if (hasVert) {
      ok = m_shader.loadFromMemory(m_vertexSrc, sf::Shader::Type::Vertex);
    }
  }

  if (!ok) {
    std::cerr << "ShaderAsset: failed to compile '" << m_metadata.name << "'"
              << std::endl;
  }

  // Release the transient CPU sources regardless of outcome.
  m_vertexSrc.clear();
  m_vertexSrc.shrink_to_fit();
  m_fragmentSrc.clear();
  m_fragmentSrc.shrink_to_fit();
  m_geometrySrc.clear();
  m_geometrySrc.shrink_to_fit();

  setState(ok ? AssetState::kLoaded : AssetState::kFailed);
  return ok;
}

bool
ShaderAsset::cookManifest(const FileSystemPath& source,
                          const FileSystemPath& sourceRoot,
                          AssetFileWriter& writer) {
  (void)sourceRoot;

  const Vector<uint8> manifestBytes = FileSystem::fastRead(source);
  if (manifestBytes.empty()) {
    return false;
  }
  String manifest;
  chunkToString(manifestBytes, manifest);

  // Stage files are named relative to the manifest's own folder.
  const FileSystemPath baseDir = source.parent_path();

  bool wroteAny = false;
  size_t lineStart = 0;
  while (lineStart <= manifest.size()) {
    size_t lineEnd = manifest.find('\n', lineStart);
    if (String::npos == lineEnd) {
      lineEnd = manifest.size();
    }
    String line = manifest.substr(lineStart, lineEnd - lineStart);
    lineStart = lineEnd + 1;

    trim(line);
    if (line.empty() || '#' == line[0]) {
      continue;
    }

    const size_t eq = line.find('=');
    if (String::npos == eq) {
      continue;
    }
    String key = line.substr(0, eq);
    String value = line.substr(eq + 1);
    trim(key);
    trim(value);
    std::transform(key.begin(), key.end(), key.begin(), [](ansichar c) {
      return static_cast<ansichar>(std::tolower(static_cast<unsigned char>(c)));
    });
    if (value.empty()) {
      continue;
    }

    ChunkFormatId stage;
    if ("vertex" == key || "vert" == key) {
      stage = ShaderChunk::kVert;
    }
    else if ("fragment" == key || "frag" == key) {
      stage = ShaderChunk::kFrag;
    }
    else if ("geometry" == key || "geom" == key) {
      stage = ShaderChunk::kGeom;
    }
    else {
      continue;
    }

    const FileSystemPath stagePath = baseDir / FileSystemPath(value);
    const Vector<uint8> stageBytes = FileSystem::fastRead(stagePath);
    if (stageBytes.empty()) {
      std::cerr << "ShaderAsset: manifest '" << source.string()
                << "' references unreadable stage '" << value << "'" << std::endl;
      continue;
    }
    writer.addChunk(stageBytes.data(), stageBytes.size(), stage);
    wroteAny = true;
  }

  return wroteAny;
}

} // namespace sfmx
