include(FetchContent)

set(LUA_BUILD_BINARY   OFF CACHE BOOL "" FORCE)
set(LUA_BUILD_COMPILER OFF CACHE BOOL "" FORCE)

FetchContent_Declare(Lua
    GIT_REPOSITORY https://github.com/walterschell/Lua.git
    GIT_TAG v5.4.7
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(Lua)

add_executable(Game main.cpp)

target_link_libraries(Game PRIVATE SFMX::FrameworkGame lua_static)
