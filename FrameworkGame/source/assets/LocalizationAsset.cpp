#include "assets/LocalizationAsset.h"

#include <sstream>

#include "assets/AssetFile.h"
#include "assets/AssetManager.h"
#include "core/FileSystem.h"

namespace sfmx
{

String LocalizationSettings::kCurrentLocalizationLanguage = "en";
Vector<String> LocalizationSettings::kAvailableLanguages = {"en", "fr", "es", "de", "cz"};

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

  // Each row after the header is a language version: the first cell is the
  // language code, the remaining cells are the values for each id in header order.
  String languageCode;
  while (std::getline(strStr, segment, '\n'))
  {
    if (segment.empty()) {
      continue;
    }
    rowStr = std::stringstream(segment);
    std::getline(rowStr, languageCode, '\t');
    int column = 0;
    while (std::getline(rowStr, celldata, '\t')) {
      if (column >= static_cast<int>(ids.size())) {
        break;  // row has more columns than the header; ignore the extras
      }
      if (!celldata.empty()) {
        m_localizations[ids[column]][languageCode] = celldata;
      }
      ++column;
    }
  }

  setState(AssetState::kLoaded);
  return true;  
}

StringView LocalizationAsset::get(StringView id) const {
  
  if (!idExists(id)) {
    return StringView(); // Return empty if id not found
  }

  const auto& languageMap = m_localizations.at(LocalizationSettings::getCurrentLanguage());
  return languageMap.at(id.data());
}

bool LocalizationAsset::idExists(StringView id) const {
  if (m_localizations.empty()) {
    return false;
  }

  if (m_localizations.count(LocalizationSettings::getCurrentLanguage()) == 0) {
    // Language was never set, should also warn about it.

    return false;
  }

  return m_localizations.at(LocalizationSettings::getCurrentLanguage()).count(id.data()) > 0;
}

} // namespace sfmx