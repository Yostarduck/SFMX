#pragma once

#include "core/platform/Prerequisites.h"
#include "assets/Asset.h"

namespace sfmx
{
class AssetFileReader;

/**
 * @brief A static class that holds the localization settings for the game
 *
 * Available everywhere so it can be changed effortlessly.
 * When setting the language, it will be used to get the localized strings from
 * a localization asset
 */
class SFMX_UTILITY_EXPORT LocalizationSettings
{
 public:
  /** @brief Sets the current localization language */
  static void setCurrentLanguage(StringView languageCode)
  {
    kCurrentLocalizationLanguage = languageCode;
  }

  /** @brief Gets the current localization language */
  static String getCurrentLanguage()
  {
    return kCurrentLocalizationLanguage;
  }

  /** @brief Gets the list of available languages */
  static const Vector<String>& getAvailableLanguages()
  {
    return kAvailableLanguages;
  }

 private:
  // The current localization id
  static String kCurrentLocalizationLanguage;
  // The list of available valid languages
  static Vector<String> kAvailableLanguages;
};


/**
 * @brief A Localization asset: holds a package of localized strings,
 *        decoded from a raw chunk (the @c .csv bytes the cooker wrapped). 
 *
 * Localization assets once loaded are only read from, they store the strings
 * in a map of maps, first key being the language id, and second the string id
 */
class SFMX_UTILITY_EXPORT LocalizationAsset : public AssetT<LocalizationAsset>
{
 public:

  /** @brief Gets the map of localized strings if you ever need to manually check one */
  NODISCARD FORCEINLINE const UnorderedMap<String, UnorderedMap<String, String>>&
  localizations() const { return m_localizations; }

  /** @brief Decodes the localization from a csv chunk 
   *  @return true on success. 
   */
  bool 
  decodeFrom(AssetFileReader& reader); 

  /** @brief Gets the localized string for a given id and a currently set language. 
   * Language is set by the LocalizationSettings setCurrentLanguage.
   * 
   * @see LocalizationSettings  
   * @return the localized string for the given id
   */
  StringView get(StringView id) const;

  /** @brief Checks if a localized string for a given id exists
   *  @return true if the string exists, false otherwise
   */
  bool idExists(StringView id) const;

 private:

  // The map of localized strings 
  UnorderedMap<String, UnorderedMap<String, String>> m_localizations;

};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::LocalizationAsset)