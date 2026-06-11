// Flags<> test group. Compile-time coverage via static_assert; a few runtime
// checks exercise the "build a flags variable, then pass it" path.

#include "FlagTest.h"

#include <iostream>

#include "core/platform/Prerequisites.h"
#include "utils/Flags.h"

#include "TestRunner.h"

using namespace sfmx;

namespace {

// ===========================================================================
// Operator coverage (compile-time)
// ===========================================================================
enum class TestFlag : uint16 {
  kNone = 0x00,
  kA    = 0x01,
  kB    = 0x02,
  kC    = 0x04,
};
SFMX_DECLARE_FLAGS_EXT(TestFlags, TestFlag, uint16)

static_assert((TestFlag::kA | TestFlag::kB).getRaw() == 0x03);
static_assert((TestFlag::kA | TestFlag::kB).isSet(TestFlag::kA));
static_assert(!(TestFlag::kA | TestFlag::kB).isSet(TestFlag::kC));
static_assert((TestFlag::kA | TestFlag::kB).isSet(TestFlag::kA | TestFlag::kB));
static_assert((TestFlag::kA | TestFlag::kB).isSetAny(TestFlag::kC | TestFlag::kA));
static_assert(!(TestFlag::kA | TestFlag::kB).isSetAny(TestFlag::kC));
static_assert(TestFlags(TestFlag::kA).set(TestFlag::kB).getRaw() == 0x03);
static_assert(TestFlags(TestFlag::kA | TestFlag::kB).unset(TestFlag::kA).getRaw() == 0x02);
static_assert((TestFlag::kA & (TestFlag::kA | TestFlag::kB)).getRaw() == 0x01);  // enum & flags
static_assert((TestFlags(TestFlag::kA) ^ TestFlag::kA).getRaw() == 0x00);
static_assert(static_cast<bool>(TestFlags(TestFlag::kA)));
static_assert(!static_cast<bool>(TestFlags(TestFlag::kNone)));
static_assert(TestFlags(TestFlag::kA) == TestFlag::kA);
static_assert(TestFlag::kA == TestFlags(TestFlag::kA));  // C++20 reversed ==
static_assert(TestFlags(TestFlag::kA) != TestFlag::kB);  // synthesized !=
static_assert(!(~TestFlags(TestFlag::kA)).isSet(TestFlag::kA));

// ===========================================================================
// Passing flags to functions (compile-time)
// ===========================================================================
NODISCARD constexpr bool
hasWriteCap(TestFlags caps) {
  return caps.isSetAny(TestFlag::kB);  // pretend kB == "write"
}

static_assert(hasWriteCap(TestFlag::kB));                 // single enumerator
static_assert(hasWriteCap(TestFlag::kA | TestFlag::kB));  // combination
static_assert(!hasWriteCap(TestFlag::kA));
static_assert(!hasWriteCap(TestFlags{}));                 // empty flags

// Raw integer only for serialization / interop, and explicit on purpose:
static constexpr uint16 kRaw = (TestFlag::kA | TestFlag::kB).getRaw();
static_assert(kRaw == 0x03);
static_assert(TestFlags(kRaw).isSet(TestFlag::kA | TestFlag::kB));

// ===========================================================================
// "createTexture(flags)" style: build a flags variable, then pass it
// ===========================================================================
enum class TextureFlag : uint32 {
  kNone         = 0,
  kSRGB         = 1 << 0,
  kGenerateMips = 1 << 1,
  kRenderTarget = 1 << 2,
};
SFMX_DECLARE_FLAGS(TextureFlags, TextureFlag)  // uint32 storage

NODISCARD constexpr bool
textureWantsMips(TextureFlags flags) {
  return flags.isSetAny(TextureFlag::kGenerateMips);
}

constexpr TextureFlags kTexFlags = TextureFlag::kSRGB | TextureFlag::kGenerateMips;
static_assert(textureWantsMips(kTexFlags));

}  // namespace

// ---------------------------------------------------------------------------
void
runFlagTests() {
  const int failedBefore = sfmxtest::g_checksFailed;

  // Runtime mirror of the "build a variable then pass it" path.
  TextureFlags flags;                     // empty
  flags.set(TextureFlag::kSRGB);          // add one
  flags |= TextureFlag::kGenerateMips;    // add another
  SFMX_CHECK(textureWantsMips(flags));    // pass the variable
  SFMX_CHECK(flags.isSet(TextureFlag::kSRGB));
  SFMX_CHECK(!flags.isSet(TextureFlag::kRenderTarget));

  // Inline at the call site also works (no temporary needed).
  SFMX_CHECK(textureWantsMips(TextureFlag::kSRGB | TextureFlag::kGenerateMips));
  SFMX_CHECK(!textureWantsMips(TextureFlag::kSRGB));

  if (sfmxtest::g_checksFailed == failedBefore) {
    std::cout << "[FlagTest] passed\n";
  }
}
