
#include "platform/Prerequisites.h"
#include "platform/STDHeaders.h"
#include <regex>

namespace sfmx
{

template<typename T, typename Tr = std::regex_traits<T>>
using BasicRegex = std::basic_regex<T, Tr>;
  
using Regex = std::regex;
  
using StringMatch = std::smatch;
  
using CharMatch = std::cmatch;

using SRegexIterator = std::sregex_iterator;

using CRegexIterator = std::cregex_iterator;

/**
 * @brief Makes a regex search with the given arguments
 * @return the object
 */
template <typename... Args>
bool regexSearch(Args&&... args) {
  return std::regex_search(std::forward<Args>(args)...);
}
  
class ArgumentParser {
 public:
  ArgumentParser() = default;

  ~ArgumentParser() = default;

  void
  parse(int argc, char **argv);

  void
  parse(StringView args);

  bool
  addFlag(StringView newFlag);

  void
  setFlagValue(StringView flag, StringView newParameter);

  const Vector<String>
  getFlagValues(StringView flag);

  bool
  hasFlag(StringView flag);

  bool
  removeFlag(StringView flag);

  Map<String, Vector<String>> m_flagMap;

};

}

