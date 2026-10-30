#include "core/ArgumentParser.h"

namespace sfmx
{


void
ArgumentParser::parse(int argc, char **argv)
{

  String currentFlag;
  Regex flagRegex(R"(-\w+)");
  Regex paramRegex(R"delim("([^"]+)"|\b(?!-)\S+\b)delim");
  // Parse arguments starting from index 1, skipping the program name (argv[0])
  for (int i = 1; i < argc; ++i) {
    String arg = argv[i];

    // Check if the argument is a flag
    if (std::regex_match(arg, flagRegex)) {
      // m_flagMap[currentFlag];  // Initialize an empty vector for this flag
      currentFlag = arg.substr(1);
      addFlag(currentFlag);
    }
    else if (!currentFlag.empty()) {  // If not a flag, treat as a parameter for the last flag
      StringMatch match;
      if (std::regex_match(arg, match, paramRegex)) {
        if (match[1].matched) {  // Multi-word parameter in quotes
          // arguments[currentFlag].push_back(match[1].str());
          setFlagValue(currentFlag, match[1].str());
        }
        else {  // Single-word parameter
          setFlagValue(currentFlag, arg);
        }
      }
    }
  }
}

void
ArgumentParser::parse(StringView parameters) {
//   String currentFlag;
// 
//   for (int i = 1; i < parameters.size(); ++i) {
//     String token; // = parameters.c_str()[i];
// 
//     if (token[0] == '-') {  // If the token is a flag
//       currentFlag = token.substr(1);  // Remove the leading '-'
//       addFlag(currentFlag);  // Initialize with an empty vector
//     }
//     else if (!currentFlag.empty()) {
//       setFlagValue(currentFlag, token);  // Store value under current flag
//     }
//   }
  String input(parameters);
  Regex flagRegex(R"(-\w+)");
  Regex paramRegex(R"delim("([^"]+)"|\b(?!-)\S+\b)delim");
  StringMatch match;
  if (regexSearch(input, match, flagRegex)) {
    String flag = match.str();
    input = match.suffix().str();  // Remaining string after the flag

    // Find parameters after the flag
    Vector<String> params;
    auto paramStart = SRegexIterator(input.begin(),
                                     input.end(), 
                                     paramRegex);
    auto paramEnd = SRegexIterator();

    for (std::sregex_iterator i = paramStart; i != paramEnd; ++i) {
      if ((*i)[1].matched) {  // Check if the quoted group matched
        params.push_back((*i)[1].str());  // Multi-word parameter
      }
      else {
        params.push_back(i->str());  // Single-word parameter
      }
    }

    // Store flag and parameters in map
    m_flagMap[flag.substr(1)] = params;
  }

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
  
  if (m_flagMap.find(flag.data()) != m_flagMap.end()) {
    if (index >= 0 && index < static_cast<int32>(m_flagMap[flag.data()].size())) {
      return m_flagMap[flag.data()][index];
    }
  }
  return {};
}



bool
ArgumentParser::hasFlag(StringView flag) {
  String toFind(flag);
  return (m_flagMap.find(toFind) != m_flagMap.end());
}

bool
ArgumentParser::removeFlag(StringView flag) {
  String toFind(flag);
  if (m_flagMap.find(toFind) != m_flagMap.end()) {
    m_flagMap.erase(m_flagMap.find(toFind));
    return true;
  }
  return false;
}

}

