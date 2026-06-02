#pragma once

#include "core/platform/Prerequisites.h"

namespace sfmx
{
class IniFile
{
public:
  IniFile();
  ~IniFile() = default;

  IniFile(const IniFile&) = delete;
  IniFile& operator=(const IniFile&) = delete;

  IniFile(IniFile&&) noexcept = default;
  IniFile& operator=(IniFile&&) noexcept = default;

  bool load(const FileSystemPath& filePath);
  bool loadAll(InitializerList<FileSystemPath> filePaths);

  NODISCARD bool has(StringView section, StringView key) const;
  NODISCARD String getString(StringView section,
                             StringView key,
                             String defaultValue = "") const;
  NODISCARD int32 getInt(StringView section,
                         StringView key,
                         int32 defaultValue = 0) const;
  NODISCARD uint32 getUInt(StringView section,
                           StringView key,
                           uint32 defaultValue = 0) const;
  NODISCARD float getFloat(StringView section,
                           StringView key,
                           float defaultValue = 0.0f) const;
  NODISCARD bool getBool(StringView section,
                         StringView key,
                         bool defaultValue = false) const;

private:
  struct Impl;
  UniquePtr<Impl> m_impl;
};
}
