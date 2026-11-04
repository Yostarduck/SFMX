#pragma once

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
 * The parser stores a map of option name -> ordered value list and keeps every
 * token that follows an option as a value of that option. It is intentionally
 * small: no schema, no type conversion, and no validation — callers ask for a
 * flag and read its values as strings.
 *
 * @par Accepted syntax
 *  - Single- or double-dash options: @c -c, @c --cook, @c --cook-scene.
 *  - Inline values: @c --name=value (quotes around @c value are stripped).
 *  - Positional values after an option: @c --cook src out. A new option ends
 *    the previous option's value list.
 *  - Quoted values, with the quotes still in the token: @c --path "Game dir"
 *    is stored as @c "Game dir" without the quotes.
 *
 * @par Names and values
 * Option names are stored without their leading dashes, so @c --cook and
 * @c -cook are both queried as @c "cook". A dash token whose first character
 * is a digit or a dot (for example @c -9.8) is treated as a value, not an
 * option, so negative numbers can be passed as arguments. Bare @c - and
 * @c -- are values too; stray values that appear before any option are
 * ignored.
 *
 * @par Lifetime
 * Values are ordinary strings owned by the parser. @ref getFlagValue returns a
 * view into that storage, so it becomes invalid when the parser is mutated
 * (@ref parse, @ref addFlag, @ref removeFlag, @ref setFlagValue).
 *
 * Repeated @ref parse calls accumulate: options parsed earlier are kept and
 * new values are appended. Create one parser per process (or per test) and
 * parse the full command line once.
 */
class ArgumentParser {
 public:
  ArgumentParser() = default;

  ~ArgumentParser() = default;

  /**
   * @brief Parse @p argv, skipping @c argv[0] (the program name).
   *
   * Each option starts a new value list; following non-option tokens are
   * appended to it. Options already parsed are kept, so repeated calls
   * accumulate.
   *
   * @param argc Argument count as passed to `main`.
   * @param argv Argument vector as passed to `main`; a null entry is treated
   *             as an empty token.
   */
  void
  parse(int argc, char **argv);

  /**
   * @brief Parse a command-line string, honouring double quotes around values.
   *
   * Tokens are split on whitespace outside double quotes. The quotes stay in
   * the token while splitting and are removed when the token is stored as a
   * value, so @c --path "Game dir" yields the single value @c "Game dir".
   * Same option syntax as @ref parse, and repeated calls accumulate in the
   * same way.
   *
   * @param args Command line without the program name.
   */
  void
  parse(StringView args);

  /**
   * @brief Register @p newFlag with an empty value list.
   *
   * @param newFlag Option name without the leading dashes.
   * @return False when the flag already existed (its values are untouched);
   *         true when it was added.
   */
  bool
  addFlag(StringView newFlag);

  /**
   * @brief Append @p newParameter to @p flag's value list.
   *
   * An empty view is still appended as an empty value (so @c --empty= yields
   * one empty value). Does nothing when @p flag is not registered; use
   * @ref addFlag to create an option that has no values yet.
   *
   * @param flag         Registered option name without the leading dashes.
   * @param newParameter Value to append.
   */
  void
  setFlagValue(StringView flag, StringView newParameter);

  /**
   * @brief Snapshot of @p flag's values in command-line order.
   *
   * Returns a copy, so it stays valid across later parser mutations.
   *
   * @param flag Option name without the leading dashes.
   * @return The stored values, or an empty vector when the flag is unknown.
   */
  const Vector<String>
  getFlagValues(StringView flag);

  /**
   * @brief Value @p index of @p flag.
   *
   * @param flag  Option name without the leading dashes.
   * @param index Zero-based position in the flag's value list.
   * @return A view into the stored value, or an empty view when the flag is
   *         unknown or @p index is negative/out of range. The view is
   *         invalidated by parser mutation.
   */
  StringView
  getFlagValue(StringView flag, int32 index);

  /**
   * @brief Query whether @p flag was seen (with or without values).
   *
   * @param flag Option name without the leading dashes.
   * @return True when the flag is registered, even if its value list is empty.
   */
  bool
  hasFlag(StringView flag);

  /**
   * @brief Remove @p flag and all of its values.
   *
   * @param flag Option name without the leading dashes.
   * @return False when the flag was not registered; true when it was erased.
   */
  bool
  removeFlag(StringView flag);

  /**
   * @brief Raw option -> value-list map.
   *
   * Public for tests and inspection; prefer the accessors above so call-site
   * intent stays readable. Keys never include the leading dashes.
   */
  Map<String, Vector<String>> m_flagMap;

 private:
  /**
   * @brief Apply one argv token (option, @c name=value pair, or value).
   *
   * Option tokens switch the "current" flag; @c name=value tokens switch it
   * and append their value immediately; other tokens are appended to the
   * current flag, or ignored when no option has been seen yet.
   *
   * @param token       The token to consume.
   * @param currentFlag The option receiving following values; updated in place
   *                    when @p token is an option.
   */
  void
  applyToken(StringView token, String& currentFlag);
};

}
