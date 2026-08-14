#include "assets/LocalizationAsset.h"

#include "assets/AssetFile.h"
#include "assets/AssetManager.h"
#include "core/FileSystem.h"

namespace sfmx
{

bool 
LocalizationAsset::decodeFrom(AssetFileReader& reader) {
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

  String data(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  m_localizations.clear();

  std::stringstream strStr(data);
  String segment;
  // Processing a CSV file separated by tabs
  // First row has all ids, each column being a new id, and each row after that is the language version
  // each row starts with the language code, and then each 
 std::getline(strStr, segment, '\n');

  // for (const auto& id : segment | std::views::split('\t')) {
  //    m_localizations[String(id.begin(), id.end())] = UnorderedMap<String, String>();
  // }

  while(std::getline(strStr, segment, '\n'))
  {
    if (!segment.empty()) {
      
    }
  }

  setState(AssetState::kLoaded);
  return true;  

} // namespace sfmx