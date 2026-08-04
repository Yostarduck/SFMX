#include "assets/AchievementAsset.h"
#include "assets/AssetFile.h"
#include "assets/AssetManager.h"

namespace sfmx
{

bool AchievementAsset::decodeFrom(AssetFileReader& reader) {
  setMetadata(reader.metadata());

  if (reader.chunkCount() == 0) {
    setState(AssetState::kFailed);
    return false;
  }

  Vector<uint8> bytes;
  if (!reader.readChunk(0, bytes) || bytes.empty()) {
    setState(AssetState::kFailed);
    return false;
  }
  
  // data is organized as follows:
  // [namesize][name][descsize][desc][iconID]
  // Read size of name, get chunk of name, and so on for description and iconID
  uint32 offset;
  uint32 nameSize;
  uint32 descSize;

  String  name;
  String  desc;
  UUID    iconID;


  if (offset + sizeof(nameSize) > bytes.size()) {
    setState(AssetState::kFailed);
    return false;
  }

  std::memcpy(&nameSize, bytes.data() + offset, sizeof(nameSize));
  offset += sizeof(nameSize);
  if (offset + nameSize > bytes.size()) {
    setState(AssetState::kFailed);
    return false;
  }
  name.resize(nameSize);
  std::memcpy(name.data(), bytes.data() + offset, nameSize);
  offset += nameSize;

  if (offset + sizeof(descSize) > bytes.size()) {
    setState(AssetState::kFailed);
    return false;
  }

  std::memcpy(&descSize, bytes.data() + offset, sizeof(descSize));
  offset += sizeof(descSize);
  if (offset + descSize > bytes.size()) {
    setState(AssetState::kFailed);
    return false;
  }
  desc.resize(descSize);
  std::memcpy(desc.data(), bytes.data() + offset, descSize);
  offset += descSize;

  if (offset + sizeof(iconID) > bytes.size()) {
    setState(AssetState::kFailed);
    return false;
  }

  std::memcpy(&iconID, bytes.data() + offset, sizeof(iconID));

}

} // namespace sfmx
