#pragma once

#include <SFML/Graphics/Shader.hpp>

#include "core/platform/Prerequisites.h"
#include "assets/Asset.h"
#include "assets/AssetMetadata.h"  // chunkFormatId

namespace sfmx
{

class AssetFileReader;
class AssetFileWriter;

/**
 * @brief Per-stage chunk tags for a cooked shader.
 *
 * A shader `.sfmxasset` carries one chunk per GLSL stage, each tagged with the id
 * for that stage so the decode can route bytes without positional assumptions. The
 * names are minted with the same by-name scheme as every other chunk format
 * (@ref chunkFormatId), so they stay stable across builds and machines.
 */
namespace ShaderChunk
{
  inline const ChunkFormatId kVert = chunkFormatId("glsl.vert");
  inline const ChunkFormatId kFrag = chunkFormatId("glsl.frag");
  inline const ChunkFormatId kGeom = chunkFormatId("glsl.geom");
} // namespace ShaderChunk

/**
 * @brief A GPU shader asset: wraps an @c sf::Shader built from one or more GLSL
 *        stage chunks (vertex / fragment / geometry).
 *
 * The @c sf::Shader is a GL resource compiled once at load time (never in the game
 * loop). Stage sources are read into transient strings on the worker thread and the
 * compile/link is deferred to @ref finalize on the GL-owning main thread, mirroring
 * how @ref TextureAsset defers its GPU upload.
 */
class SFMX_UTILITY_EXPORT ShaderAsset : public AssetT<ShaderAsset>
{
 public:
  NODISCARD FORCEINLINE const sf::Shader&
  shader() const { return m_shader; }

  NODISCARD FORCEINLINE sf::Shader&
  shader() { return m_shader; }

  bool
  decodeFrom(AssetFileReader& reader) override;

  bool
  decodeCPU(AssetFileReader& reader) override;

  bool
  finalize() override;

  /**
   * @brief Cook a `.shader` manifest into a multi-chunk `.sfmxasset`.
   *
   * The manifest is line-oriented `stage=relative/path.glsl` (stage is one of
   * `vertex`, `fragment`, `geometry`); each referenced file is read and written as
   * its own stage-tagged chunk. Registered as the cook hook for `.shader` in the
   * @ref AssetImporterRegistry. Returns false (file skipped) if no stage resolved.
   */
  static bool
  cookManifest(const FileSystemPath& source,
               const FileSystemPath& sourceRoot,
               AssetFileWriter& writer);

 private:
  sf::Shader m_shader;
  // CPU-side stage sources, alive only between decodeCPU (worker) and finalize
  // (main); released after the compile. Empty at rest.
  String m_vertexSrc;
  String m_fragmentSrc;
  String m_geometrySrc;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::ShaderAsset)
