#pragma once

#include <cstddef>
#include <cstdint>

#include "core/platform/PlatformDefines.h"

namespace sfmx
{
using std::int8_t;
using std::int16_t;
using std::int32_t;
using std::int64_t;
using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

using wchar16 = char16_t;
using wchar32 = char32_t;
using ansichar = char;
using unichar = wchar16;
using unchar = unsigned char;

using typeOfNull = int32;
using size_t = std::size_t;
}
