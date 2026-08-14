#include "assets/AchievementAsset.h"
#include "assets/AssetFile.h"
#include "assets/AssetManager.h"
#include <ranges>

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
  

  // We only store in the achievement asset the id of the achievement, each achievement is a new line
  // Localization file has the name and description of the achievement, we will load this data later
  String data(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  m_ids.clear();

  std::stringstream strStr(data);
  String segment;

  while(std::getline(strStr, segment, '\n'))
  {
    if (!segment.empty()) {
      m_ids.push_back(segment);
    }
  }

  setState(AssetState::kLoaded);
  return true;  
}

} // namespace sfmx
