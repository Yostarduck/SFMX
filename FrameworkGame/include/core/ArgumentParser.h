
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
  
/**
 * @brief Minimal command-line option parser.
 *
 * Accepts the single- and double-dash forms of an option (@c -c, @c --cook,
 * @c --cook-scene) as well as the inline @c --name=value form. Every token that
 * follows an option and is not itself an option becomes a value of that option;
 * starting a new option starts a new value list. Quoted values (quotes still in
 * the token) are stored without the surrounding quotes.
 *
 * Option names are stored without their leading dashes, so @c --cook and
 * @c -cook are both queried as @c "cook". A dash token whose first character is
 * a digit or a dot (e.g. @c -9.8) is treated as a value, not an option, so
 * negative numbers can be passed as arguments.
 */
class ArgumentParser {
 public:
  ArgumentParser() = default;

  ~ArgumentParser() = default;

  /**
   * @brief Parse @p argv (skipping @c argv[0], the program name).
   *
   * Options already parsed are kept; repeated calls accumulate.
   */
  void
  parse(int argc, char **argv);

  /**
   * @brief Parse a command-line string, honouring double quotes around values.
   *
   * Same option syntax as @ref parse(int, char**).
   */
  void
  parse(StringView args);

  /** @brief Register @p newFlag with an empty value list. @return False if it already existed. */
  bool
  addFlag(StringView newFlag);

  /** @brief Append a value to an already registered flag (no-op for unknown flags). */
  void
  setFlagValue(StringView flag, StringView newParameter);

  /** @brief Snapshot of a flag's values (empty when the flag is unknown). */
  const Vector<String>
  getFlagValues(StringView flag);

  /** @brief Value @p index of @p flag, or an empty view when absent/out of range. */
  StringView
  getFlagValue(StringView flag, int32 index);

  bool
  hasFlag(StringView flag);

  /** @brief Remove @p flag and all its values. @return False if it was not registered. */
  bool
  removeFlag(StringView flag);

  Map<String, Vector<String>> m_flagMap;

 private:
  /**
   * @brief Apply one argv token (option, @c name=value pair, or value).
   * @param token       The token to consume.
   * @param currentFlag The option receiving following values; updated in place.
   */
  void
  applyToken(StringView token, String& currentFlag);
};

}

