extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <cstdio>

int main() {
  // Create new Lua state
  lua_State *L = luaL_newstate();
  
  // Load standard libraries
  luaL_openlibs(L);
  
  // Execute simple Lua code
  luaL_dostring(L, "print('Hello from Lua!')");
  
  // Clean up
  lua_close(L);
  return 0;
}