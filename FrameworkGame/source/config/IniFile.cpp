#include "config/IniFile.h"

#include "core/platform/PlatformTypes.h"
#include "core/FileSystem.h"
#include "mini/ini.h"

namespace
{
using namespace sfmx;

template <typename T>
bool parseNumber(const String& value, T& output)
{
  const char* begin = value.data();
  const char* end = value.data() + value.size();
  const auto [ptr, ec] = std::from_chars(begin, end, output);
  return ec == std::errc() && ptr == end;
}

String toLower(String value)
{
  std::transform(value.begin(),
                 value.end(),
                 value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}
}

namespace sfmx
{
struct IniFile::Impl
{
  mINI::INIStructure data;
};

IniFile::IniFile() : m_impl(MakeUnique<Impl>()){}
IniFile::~IniFile() = default;

bool IniFile::load(const FileSystemPath& filePath)
{
  // Relative paths resolve under the content root (exe dir at runtime).
  mINI::INIFile file(FileSystem::resolve(filePath).string());
  mINI::INIStructure incoming;

  if (!file.read(incoming))
  {
    return false;
  }

  for (const auto& sectionPair : incoming)
  {
    auto& section = m_impl->data[sectionPair.first];
    for (const auto& keyPair : sectionPair.second)
    {
      section[keyPair.first] = keyPair.second;
    }
  }

  return true;
}

bool IniFile::loadAll(InitializerList<FileSystemPath> filePaths)
{
  bool result = true;
  for (const auto& filePath : filePaths)
  {
    if (!load(filePath))
    {
      result = false;
    }
  }
  return result;
}

bool IniFile::has(StringView section, StringView key) const
{
  const String sectionKey(section);
  const String valueKey(key);

  if (!m_impl->data.has(sectionKey))
  {
    return false;
  }

  return m_impl->data.get(sectionKey).has(valueKey);
}

String IniFile::getString(StringView section,
                          StringView key,
                          String defaultValue) const
{
  const String sectionKey(section);
  const String valueKey(key);

  if (!m_impl->data.has(sectionKey))
  {
    return defaultValue;
  }

  const auto sectionData = m_impl->data.get(sectionKey);
  if (!sectionData.has(valueKey))
  {
    return defaultValue;
  }

  return sectionData.get(valueKey);
}

int32 IniFile::getInt(StringView section, StringView key, int32 defaultValue) const
{
  const String value = getString(section, key);
  int32 parsedValue = defaultValue;
  if (!parseNumber(value, parsedValue))
  {
    return defaultValue;
  }
  return parsedValue;
}

uint32 IniFile::getUInt(StringView section,
                        StringView key,
                        uint32 defaultValue) const
{
  const String value = getString(section, key);
  uint32 parsedValue = defaultValue;
  if (!parseNumber(value, parsedValue))
  {
    return defaultValue;
  }
  return parsedValue;
}

float IniFile::getFloat(StringView section, StringView key, float defaultValue) const
{
  const String value = getString(section, key);
  float parsedValue = defaultValue;
  if (!parseNumber(value, parsedValue))
  {
    return defaultValue;
  }
  return parsedValue;
}

bool IniFile::getBool(StringView section, StringView key, bool defaultValue) const
{
  const String value = toLower(getString(section, key));

  if (value == "1" || value == "true" || value == "yes" || value == "on")
  {
    return true;
  }

  if (value == "0" || value == "false" || value == "no" || value == "off")
  {
    return false;
  }

  return defaultValue;
}
}
