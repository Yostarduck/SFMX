# LuaJIT is fetched the same way SFML is in SketchGfx: cloned at configure time
# via FetchContent. Upstream LuaJIT ships only Makefiles (and an MSVC batch
# script), so we pair the upstream source with zhaozg/luajit-cmake, a CMake
# build wrapper that bootstraps buildvm natively -- no GNU make required, so it
# works with MSVC. The wrapper exposes the luajit::lib and luajit::header
# targets.
include(FetchContent)

# Upstream LuaJIT source. It has no CMakeLists of its own, so MakeAvailable only
# populates it -- nothing is configured here. Pinned to the v2.1 branch HEAD.
FetchContent_Declare(luajit_src
    GIT_REPOSITORY https://github.com/LuaJIT/LuaJIT.git
    GIT_TAG b925b3e3fc6771171602323b45fbe9fb8fc90369
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(luajit_src)

# CMake build wrapper; point it at the upstream source fetched above. We only
# need the library, not the luajit command-line interpreter.
set(LUAJIT_DIR       "${luajit_src_SOURCE_DIR}" CACHE PATH "" FORCE)
set(LUAJIT_BUILD_EXE OFF CACHE BOOL "" FORCE)
FetchContent_Declare(luajit_cmake
    GIT_REPOSITORY https://github.com/zhaozg/luajit-cmake.git
    GIT_TAG bdc1b54d0572e2cc5c3eaa5c1e818d0e0246b9d1
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(luajit_cmake)

# sol2: a header-only C++ <-> Lua binding layer used on top of LuaJIT. Fetched
# the same way as everything else. With the default options (tests/examples
# off) MakeAvailable only defines the header-only sol2::sol2 target; it does
# not search for or build its own Lua, so it binds against the LuaJIT above.
FetchContent_Declare(sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG v3.5.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(sol2)

add_executable(Game main.cpp)

# sol2 includes LuaJIT's <lua.hpp> via luajit::header. SOL_LUAJIT=1 makes sol2's
# LuaJIT-specific code path explicit rather than relying on auto-detection.
target_link_libraries(Game PRIVATE
    SFMX::FrameworkGame luajit::lib luajit::header sol2::sol2)
target_compile_definitions(Game PRIVATE SOL_LUAJIT=1)

# luajit.h is generated at build time (it embeds the version) into the wrapper's
# binary dir, which luajit::header does not expose. LuaJIT's own <lua.hpp> -- and
# therefore sol2 -- includes it, so put that dir on the include path.
target_include_directories(Game PRIVATE ${luajit_cmake_BINARY_DIR})
