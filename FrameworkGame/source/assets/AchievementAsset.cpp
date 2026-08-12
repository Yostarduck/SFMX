#include "assets/AchievementAsset.h"
#include "assets/AssetFile.h"
#include "assets/AssetManager.h"
#include "rapidjson/rapidjson.h"
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
  
  rapidjson::Document doc;
  if (doc.Parse(reinterpret_cast<const char*>(bytes.data()), bytes.size()).HasParseError()) {
    setState(AssetState::kFailed);
    return false;
  }


  doc["name"][""].GetString();

  // data is organized as follows:
  // [namesize][name][descsize][desc][iconID]
  // Read size of name, get chunk of name, and so on for description and iconID
  // uint32 offset = 0;
  // uint32 nameSize = 0;
  // uint32 descSize = 0;
  // String  name = "";
  // String  desc = "";
  // UUID    iconID = UUID::null();
  // if (offset + sizeof(nameSize) > bytes.size()) {
  //   setState(AssetState::kFailed);
  //   return false;
  // }
  // std::memcpy(&nameSize, bytes.data() + offset, sizeof(nameSize));
  // offset += sizeof(nameSize);
  // if (offset + nameSize > bytes.size()) {
  //   setState(AssetState::kFailed);
  //   return false;
  // }
  // name.resize(nameSize);
  // std::memcpy(name.data(), bytes.data() + offset, nameSize);
  // offset += nameSize;
  // if (offset + sizeof(descSize) > bytes.size()) {
  //   setState(AssetState::kFailed);
  //   return false;
  // }
  // std::memcpy(&descSize, bytes.data() + offset, sizeof(descSize));
  // offset += sizeof(descSize);
  // if (offset + descSize > bytes.size()) {
  //   setState(AssetState::kFailed);
  //   return false;
  // }
  // desc.resize(descSize);
  // std::memcpy(desc.data(), bytes.data() + offset, descSize);
  // offset += descSize;
// 
  // if (offset + sizeof(iconID) > bytes.size()) {
  //   setState(AssetState::kFailed);
  //   return false;
  // }
// 
  // std::memcpy(&iconID, bytes.data() + offset, sizeof(iconID));

}

} // namespace sfmx
