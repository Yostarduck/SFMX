#pragma once

#include <filesystem>

namespace sfmx
{
// TODO: this will become an engine-owned Path type so <filesystem> stays hidden
// behind it. For now it aliases std::filesystem::path.
using FileSystemPath = std::filesystem::path;
// NOTE: std::filesystem is wrapped by the sfmx::FileSystem class (core/FileSystem.h);
// do not add a `namespace FileSystem = std::filesystem` alias here (name clash).
} // namespace sfmx
