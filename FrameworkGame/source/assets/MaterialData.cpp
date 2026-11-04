#include "assets/MaterialData.h"

#include <utility>

#include "core/DataStreamTypes.h"

namespace sfmx
{

void
MaterialData::serialize(DataStream& stream) const {
  stream << kVersion;
  stream << shaderAssetId;

  stream << static_cast<uint64>(uniforms.size());
  for (const MaterialUniform& uniform : uniforms) {
    stream.writeString(uniform.name);
    stream << static_cast<uint8>(uniform.kind);
    stream << uniform.value[0] << uniform.value[1] << uniform.value[2]
           << uniform.value[3];
  }

  stream.writeString(textureUniform);
  stream << static_cast<uint8>(bindCurrentTexture ? 1 : 0);
}

bool
MaterialData::deserialize(DataStream& stream) {
  uint32 version = 0;
  stream >> version;
  if (kVersion != version) {
    return false;
  }

  UUID shaderId;
  stream >> shaderId;

  uint64 count = 0;
  stream >> count;
  if (count > kMaxUniforms) {
    return false;
  }

  Vector<MaterialUniform> readUniforms;
  readUniforms.reserve(static_cast<size_t>(count));
  for (uint64 i = 0; i < count; ++i) {
    MaterialUniform uniform;
    uniform.name = stream.readString();
    uint8 kind = 0;
    stream >> kind;
    uniform.kind = static_cast<MaterialUniformKind>(kind);
    stream >> uniform.value[0] >> uniform.value[1] >> uniform.value[2]
           >> uniform.value[3];
    readUniforms.push_back(std::move(uniform));
  }

  String texture = stream.readString();
  uint8 bindCurrentTextureByte = 1;
  stream >> bindCurrentTextureByte;

  shaderAssetId       = shaderId;
  uniforms            = std::move(readUniforms);
  textureUniform      = std::move(texture);
  bindCurrentTexture  = (bindCurrentTextureByte != 0);
  return true;
}

} // namespace sfmx
