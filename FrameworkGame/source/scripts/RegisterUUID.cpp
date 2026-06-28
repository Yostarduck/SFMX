#include "scripts/RegisterUUID.h"

#include <functional>

#include "core/platform/Prerequisites.h"
#include "utils/UUID.h"

namespace sfmx
{

namespace script
{

void
registerUUID(sol::state_view lua) {
  lua.new_usertype<UUID>("UUID",
    sol::call_constructor,
    sol::constructors<UUID()>(),

    "isNull", &UUID::isNull,
    "toString", &UUID::toString,
    "getHash", &UUID::getHash,
    
    sol::meta_function::equal_to,
    [](const UUID& left, const UUID& right) {
      return left == right;
    },

    sol::meta_function::less_than,
    [](const UUID& left, const UUID& right) {
      return left < right;
    },
    

    "createFromName", sol::overload(
      [](const String& name) { return UUID::createFromName(name); },
      [](const String& name, const UUID& ns) {
        return UUID::createFromName(name, ns);
      }
    ),

    "null", []() -> const UUID { return UUID::null(); }
  );
}

}  // namespace script

}  // namespace sfmx