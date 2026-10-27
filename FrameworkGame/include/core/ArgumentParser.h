
#include "platform/Prerequisites.h"
#include "platform/STDHeaders.h"

namespace sfmx
{

class ArgumentParser {
 public:
  ArgumentParser() = default;
  
  ~ArgumentParser() = default;

  void
  parse(int argc, char **argv);

  void
  parse(StringView args);

  bool
  addFlag(Stringview newFlag);

  void
  setFlagValue(Stringview flag, Stringview newParameter);

  const Vector<String>
  getFlagValues(Stringview flag);

  bool
  hasFlag(Stringview flag);

  bool
  removeFlag(Stringview flag);

  Map<String, Vector<String>> m_flagMap;

};

}

