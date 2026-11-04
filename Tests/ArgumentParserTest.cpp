// doctest only forward-declares std::ostream; the string_view comparisons below
// need the full definition for its stringifier.
#include <ostream>

#include <doctest/doctest.h>

#include <initializer_list>

#include "core/platform/Prerequisites.h"
#include "core/ArgumentParser.h"

using namespace sfmx;

namespace {

// Builds a writable argv (the parser only reads it) and parses it.
void
parseArgs(ArgumentParser& parser, std::initializer_list<const char*> args) {
  Vector<String> storage;
  storage.reserve(args.size() + 1);
  storage.emplace_back("Game"); // argv[0]: skipped by the parser
  for (const char* arg : args) {
    storage.emplace_back(nullptr == arg ? "" : arg);
  }

  Vector<char*> argv;
  argv.reserve(storage.size());
  for (String& arg : storage) {
    argv.push_back(arg.data());
  }

  parser.parse(static_cast<int>(argv.size()), argv.data());
}

} // namespace

TEST_CASE("ArgumentParser - long options accept values") {
  ArgumentParser parser;
  parseArgs(parser, {"--cook", "Game/resources", "Game/assets"});

  CHECK(parser.hasFlag("cook"));
  CHECK(parser.getFlagValues("cook").size() == 2);
  CHECK(parser.getFlagValue("cook", 0) == "Game/resources");
  CHECK(parser.getFlagValue("cook", 1) == "Game/assets");
}

TEST_CASE("ArgumentParser - single and double dash share the flag name") {
  ArgumentParser parser;
  parseArgs(parser, {"-cook", "--cook-scene", "--content-dir", "Game"});

  CHECK(parser.hasFlag("cook"));
  CHECK(parser.hasFlag("cook-scene"));
  CHECK(parser.hasFlag("content-dir"));
  CHECK(parser.getFlagValues("cook").empty());
  CHECK(parser.getFlagValue("content-dir", 0) == "Game");
}

TEST_CASE("ArgumentParser - an option starts a new value list") {
  ArgumentParser parser;
  parseArgs(parser, {"--input", "a", "b", "--output", "c", "--flag"});

  CHECK(parser.getFlagValues("input").size() == 2);
  CHECK(parser.getFlagValue("input", 0) == "a");
  CHECK(parser.getFlagValue("input", 1) == "b");
  CHECK(parser.getFlagValues("output").size() == 1);
  CHECK(parser.getFlagValue("output", 0) == "c");
  CHECK(parser.getFlagValues("flag").empty());
}

TEST_CASE("ArgumentParser - --name=value form") {
  ArgumentParser parser;
  parseArgs(parser, {"--content-dir=Game/resources", "--title=\"My Game\"", "--empty="});

  CHECK(parser.getFlagValue("content-dir", 0) == "Game/resources");
  CHECK(parser.getFlagValue("title", 0) == "My Game");
  REQUIRE(parser.getFlagValues("empty").size() == 1);
  CHECK(parser.getFlagValue("empty", 0).empty());
}

TEST_CASE("ArgumentParser - quoted values are stored without quotes") {
  ArgumentParser parser;
  parseArgs(parser, {"--path", "\"Game/resources dir\"", "\"Game/assets dir\""});

  CHECK(parser.getFlagValues("path").size() == 2);
  CHECK(parser.getFlagValue("path", 0) == "Game/resources dir");
  CHECK(parser.getFlagValue("path", 1) == "Game/assets dir");
}

TEST_CASE("ArgumentParser - negative numbers and bare dashes are values") {
  ArgumentParser parser;
  parseArgs(parser, {"--gravity", "-9.8", "--scale", "-.5", "--stdin", "-"});

  CHECK(parser.hasFlag("gravity"));
  CHECK(parser.getFlagValue("gravity", 0) == "-9.8");
  CHECK(parser.getFlagValue("scale", 0) == "-.5");
  CHECK(parser.getFlagValue("stdin", 0) == "-");
  CHECK_FALSE(parser.hasFlag("9.8"));
}

TEST_CASE("ArgumentParser - stray values without an option are ignored") {
  ArgumentParser parser;
  parseArgs(parser, {"stray", "--flag", "x"});

  CHECK_FALSE(parser.hasFlag("stray"));
  CHECK(parser.getFlagValue("flag", 0) == "x");
}

TEST_CASE("ArgumentParser - bare dash tokens are not options") {
  ArgumentParser parser;
  parseArgs(parser, {"-", "--", "---", "--flag"});

  CHECK(parser.m_flagMap.size() == 1);
  CHECK(parser.hasFlag("flag"));
}

TEST_CASE("ArgumentParser - parse(StringView) handles multiple options") {
  ArgumentParser parser;
  parser.parse(R"(--cook "Game/resources dir" Game/assets --cook-scene --verbose)");

  CHECK(parser.hasFlag("cook"));
  CHECK(parser.getFlagValues("cook").size() == 2);
  CHECK(parser.getFlagValue("cook", 0) == "Game/resources dir");
  CHECK(parser.getFlagValue("cook", 1) == "Game/assets");
  CHECK(parser.hasFlag("cook-scene"));
  CHECK(parser.hasFlag("verbose"));
}

TEST_CASE("ArgumentParser - flag queries, bounds and removal") {
  ArgumentParser parser;
  parseArgs(parser, {"--foo", "a"});

  CHECK(parser.hasFlag("foo"));
  CHECK_FALSE(parser.hasFlag("nope"));
  CHECK(parser.getFlagValue("foo", 1).empty());
  CHECK(parser.getFlagValue("foo", -1).empty());
  CHECK(parser.getFlagValue("nope", 0).empty());
  CHECK(parser.getFlagValues("nope").empty());

  CHECK(parser.addFlag("bar"));
  CHECK_FALSE(parser.addFlag("bar"));

  CHECK(parser.removeFlag("foo"));
  CHECK_FALSE(parser.hasFlag("foo"));
  CHECK_FALSE(parser.removeFlag("foo"));
}
