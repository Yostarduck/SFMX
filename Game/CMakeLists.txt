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

add_executable(Game main.cpp)

target_link_libraries(Game PRIVATE SFMX::FrameworkGame luajit::lib luajit::header)
