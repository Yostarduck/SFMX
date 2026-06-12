#include <doctest/doctest.h>

#include "core/platform/Prerequisites.h"
#include "utils/Flags.h"

using namespace sfmx;

namespace {

// ===========================================================================
// Operator coverage (compile-time + runtime)
// ===========================================================================
enum class TestFlag : uint16 {
  kNone = 0x00,
  kA    = 0x01,
  kB    = 0x02,
  kC    = 0x04,
};
SFMX_DECLARE_FLAGS_EXT(TestFlags, TestFlag, uint16)

// "createTexture(flags)" style
enum class TextureFlag : uint32 {
  kNone         = 0,
  kSRGB         = 1 << 0,
  kGenerateMips = 1 << 1,
  kRenderTarget = 1 << 2,
};
SFMX_DECLARE_FLAGS(TextureFlags, TextureFlag)

NODISCARD constexpr bool
textureWantsMips(TextureFlags flags) {
  return flags.isSetAny(TextureFlag::kGenerateMips);
}

}  // namespace

// ---------------------------------------------------------------------------
// Compile-time verification (static_assert)
// ---------------------------------------------------------------------------
static_assert((TestFlag::kA | TestFlag::kB).getRaw() == 0x03);
static_assert((TestFlag::kA | TestFlag::kB).isSet(TestFlag::kA));
static_assert(!(TestFlag::kA | TestFlag::kB).isSet(TestFlag::kC));
static_assert((TestFlag::kA | TestFlag::kB).isSet(TestFlag::kA | TestFlag::kB));
static_assert((TestFlag::kA | TestFlag::kB).isSetAny(TestFlag::kC | TestFlag::kA));
static_assert(!(TestFlag::kA | TestFlag::kB).isSetAny(TestFlag::kC));
static_assert(TestFlags(TestFlag::kA).set(TestFlag::kB).getRaw() == 0x03);
static_assert(TestFlags(TestFlag::kA | TestFlag::kB).unset(TestFlag::kA).getRaw() == 0x02);
static_assert((TestFlag::kA & (TestFlag::kA | TestFlag::kB)).getRaw() == 0x01);
static_assert((TestFlags(TestFlag::kA) ^ TestFlag::kA).getRaw() == 0x00);
static_assert(static_cast<bool>(TestFlags(TestFlag::kA)));
static_assert(!static_cast<bool>(TestFlags(TestFlag::kNone)));
static_assert(TestFlags(TestFlag::kA) == TestFlag::kA);
static_assert(TestFlag::kA == TestFlags(TestFlag::kA));
static_assert(TestFlags(TestFlag::kA) != TestFlag::kB);
static_assert(!(~TestFlags(TestFlag::kA)).isSet(TestFlag::kA));
static_assert(textureWantsMips(TextureFlag::kSRGB | TextureFlag::kGenerateMips));
static_assert(!textureWantsMips(TextureFlag::kSRGB));
static_assert(textureWantsMips(TextureFlag::kGenerateMips));
static_assert(!textureWantsMips(TextureFlag::kNone));
constexpr TextureFlags kTexFlags = TextureFlag::kSRGB | TextureFlag::kGenerateMips;
static_assert(textureWantsMips(kTexFlags));

// ---------------------------------------------------------------------------
// Runtime verification
// ---------------------------------------------------------------------------
TEST_CASE("Flags::operator| combines bits") {
  CHECK((TestFlag::kA | TestFlag::kB).getRaw() == 0x03);
}

TEST_CASE("Flags::isSet single bit") {
  CHECK((TestFlag::kA | TestFlag::kB).isSet(TestFlag::kA));
  CHECK_FALSE((TestFlag::kA | TestFlag::kB).isSet(TestFlag::kC));
  CHECK((TestFlag::kA | TestFlag::kB).isSet(TestFlag::kA | TestFlag::kB));
}

TEST_CASE("Flags::isSetAny") {
  CHECK((TestFlag::kA | TestFlag::kB).isSetAny(TestFlag::kC | TestFlag::kA));
  CHECK_FALSE((TestFlag::kA | TestFlag::kB).isSetAny(TestFlag::kC));
}

TEST_CASE("Flags::set / unset") {
  CHECK(TestFlags(TestFlag::kA).set(TestFlag::kB).getRaw() == 0x03);
  CHECK(TestFlags(TestFlag::kA | TestFlag::kB).unset(TestFlag::kA).getRaw() == 0x02);
}

TEST_CASE("Flags &, ^, ~, bool, ==") {
  CHECK((TestFlag::kA & (TestFlag::kA | TestFlag::kB)).getRaw() == 0x01);
  CHECK((TestFlags(TestFlag::kA) ^ TestFlag::kA).getRaw() == 0x00);
  CHECK(static_cast<bool>(TestFlags(TestFlag::kA)));
  CHECK_FALSE(static_cast<bool>(TestFlags(TestFlag::kNone)));
  CHECK(TestFlags(TestFlag::kA) == TestFlag::kA);
  CHECK(TestFlag::kA == TestFlags(TestFlag::kA));
  CHECK(TestFlags(TestFlag::kA) != TestFlag::kB);
  CHECK_FALSE((~TestFlags(TestFlag::kA)).isSet(TestFlag::kA));
}

TEST_CASE("Flags passed to functions") {
  CHECK(textureWantsMips(TextureFlag::kSRGB | TextureFlag::kGenerateMips));
  CHECK_FALSE(textureWantsMips(TextureFlag::kSRGB));
}

TEST_CASE("Flags built as variable then passed") {
  TextureFlags flags;
  flags.set(TextureFlag::kSRGB);
  flags |= TextureFlag::kGenerateMips;
  CHECK(textureWantsMips(flags));
  CHECK(flags.isSet(TextureFlag::kSRGB));
  CHECK_FALSE(flags.isSet(TextureFlag::kRenderTarget));
}
