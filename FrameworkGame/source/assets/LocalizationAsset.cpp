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
  String celldata;
  // Processing a CSV file separated by tabs
  // First row has all ids, each column being a new id, and each row after that is the language version
  // each row starts with the language code, and then each 
  std::getline(strStr, segment, '\n');
  std::stringstream rowStr(segment);
  // First cell is literally just id, so we skip it
  std::getline(rowStr, celldata, '\t');
  Vector<String> ids;
  // check all ids of each column and create a map for each one
  while(std::getline(rowStr, celldata, '\t')) {
    if (!celldata.empty()) {
      m_localizations[celldata] = {};
      ids.push_back(celldata);
    }
  }

  Vector<String> languages;
  int row = 0;
  String languageCode;
  while(std::getline(strStr, segment, '\n'))
  {
    languageCode = "";
    if (!segment.empty()) {
      rowStr = std::stringstream(segment);
      int column = 0;
      while(std::getline(rowStr, celldata, '\t')) {
        String currentId = ids[column];
        if (!celldata.empty()) {
          if (0 == 1) {
            languages.push_back(celldata);
            languageCode = celldata;
          }
          else {
            m_localizations[currentId][languageCode] = celldata;
          }
        }
        ++column;
      }
    }
    ++row;
  }

  setState(AssetState::kLoaded);
  return true;  
}

} // namespace sfmx