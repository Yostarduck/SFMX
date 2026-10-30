#include "core/ArgumentParser.h"

#include <cctype>

namespace sfmx
{

namespace
{

/**
 * @brief True for an option token: @c -c, @c --cook, @c --cook-scene.
 *
 * One or two leading dashes are followed by a letter or underscore, then any
 * number of word characters or dashes. Bare @c - and @c -- are not options, and
 * a dash token starting with a digit or dot (@c -9.8) is a value, so negative
 * numbers can be passed as arguments.
 */
bool
isFlag(StringView token) {
  if (token.size() < 2 || '-' != token[0]) {
    return false;
  }
  size_t i = 1;
  if (i < token.size() && '-' == token[i]) {
    ++i;
  }
  if (i >= token.size()) {
    return false;
  }
  const unsigned char first = static_cast<unsigned char>(token[i]);
  if (!std::isalpha(first) && '_' != token[i]) {
    return false;
  }
  for (; i < token.size(); ++i) {
    const unsigned char c = static_cast<unsigned char>(token[i]);
    if (!std::isalnum(c) && '_' != c && '-' != c) {
      return false;
    }
  }
  return true;
}

/** @brief The option name without its leading dash(es). Precondition: isFlag(). */
StringView
flagName(StringView token) {
  const size_t first = token.find_first_not_of('-');
  return StringView::npos == first ? StringView{} : token.substr(first);
}

/** @brief Strip one pair of surrounding double quotes, if present. */
StringView
unquote(StringView value) {
  if (value.size() >= 2 && '"' == value.front() && '"' == value.back()) {
    return value.substr(1, value.size() - 2);
  }
  return value;
}

} // namespace

void
ArgumentParser::parse(int argc, char **argv) {
  String currentFlag;
  for (int i = 1; i < argc; ++i) {
    const StringView token = (nullptr == argv[i]) ? StringView{} : StringView(argv[i]);
    applyToken(token, currentFlag);
  }
}

void
ArgumentParser::parse(StringView parameters) {
  String currentFlag;
  String token;
  bool inQuotes = false;

  const auto flush = [&]() {
    if (!token.empty()) {
      applyToken(token, currentFlag);
      token.clear();
    }
  };

  for (const char c : parameters) {
    if ('"' == c) {
      // Keep the quotes in the token; unquote() strips the pair once it becomes
      // a value (an option can also carry quotes in its name=value form).
      inQuotes = !inQuotes;
      token.push_back(c);
    }
    else if (!inQuotes && std::isspace(static_cast<unsigned char>(c))) {
      flush();
    }
    else {
      token.push_back(c);
    }
  }
  flush();
}

void
ArgumentParser::applyToken(StringView token, String& currentFlag) {
  // Inline --name=value form (checked first: '=' is not a flag character).
  const size_t equals = token.find('=');
  if (StringView::npos != equals && isFlag(token.substr(0, equals))) {
    const String flag(flagName(token.substr(0, equals)));
    addFlag(flag);
    setFlagValue(flag, unquote(token.substr(equals + 1)));
    currentFlag = flag;
    return;
  }

  if (isFlag(token)) {
    currentFlag = String(flagName(token));
    addFlag(currentFlag);
    return;
  }

  if (currentFlag.empty()) {
    return; // Stray value with no owning option: ignore it.
  }

  setFlagValue(currentFlag, unquote(token));
}

bool
ArgumentParser::addFlag(StringView newFlag) {
  String toFind(newFlag);
  if (m_flagMap.find(toFind) == m_flagMap.end()) {
    m_flagMap[toFind] = {};
    return true;
  }
  return false;
}

void
ArgumentParser::setFlagValue(StringView flag, StringView newParameter) {
  String toFind(flag);
  String param(newParameter);
  if (m_flagMap.find(toFind) != m_flagMap.end()) {
    m_flagMap[toFind].push_back(param);
  }
}

const Vector<String>
ArgumentParser::getFlagValues(StringView flag) {
  String toFind(flag);
  if (m_flagMap.find(toFind) != m_flagMap.end()) {
    return m_flagMap[toFind];
  }
  return {};
}

StringView
ArgumentParser::getFlagValue(StringView flag, int32 index) {
  const String toFind(flag);
  const auto flagIt = m_flagMap.find(toFind);
  if (flagIt != m_flagMap.end() && index >= 0 &&
      index < static_cast<int32>(flagIt->second.size())) {
    return flagIt->second[static_cast<size_t>(index)];
  }
  return {};
}

bool
ArgumentParser::hasFlag(StringView flag) {
  const String toFind(flag);
  return m_flagMap.find(toFind) != m_flagMap.end();
}

bool
ArgumentParser::removeFlag(StringView flag) {
  const String toFind(flag);
  const auto it = m_flagMap.find(toFind);
  if (it != m_flagMap.end()) {
    m_flagMap.erase(it);
    return true;
  }
  return false;
}

} // namespace sfmx
