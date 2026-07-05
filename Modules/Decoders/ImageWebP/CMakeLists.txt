# SFMX::ImageWebP — the first format module (Modules/Decoders/). Static lib now; it
# registers its decoder + import rule into the core's registries at runtime (the same
# seam a DLL plugin would use later). The core (FrameworkGame) never links libwebp —
# only this module does (PRIVATE), so the format library stays out of the core.

include(FetchContent)

# libwebp is multi-file with its own CMake, so it is FETCHED (not vendored). We
# only need decode, so disable the tools/extras/mux and link the decode-only
# `webpdecoder` target.
set(WEBP_BUILD_ANIM_UTILS OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_CWEBP      OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_DWEBP      OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_GIF2WEBP   OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_IMG2WEBP   OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_VWEBP      OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_WEBPINFO   OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_WEBPMUX    OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_EXTRAS     OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_LIBWEBPMUX OFF CACHE BOOL "" FORCE)
set(WEBP_LINK_STATIC      ON  CACHE BOOL "" FORCE)
FetchContent_Declare(libwebp
    GIT_REPOSITORY https://github.com/webmproject/libwebp.git
    GIT_TAG v1.4.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(libwebp)

add_library(ImageWebP STATIC
    source/WebPImageDecoder.cpp)

add_library(SFMX::ImageWebP ALIAS ImageWebP)

target_include_directories(ImageWebP
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    # libwebp's public headers live under <src>/webp/*.h; the webpdecoder target does
    # not export this, so point at it ourselves. PRIVATE: the webp headers are an
    # implementation detail of this module, never leaked to the core or the Game exe.
    PRIVATE ${libwebp_SOURCE_DIR}/src)

# Same static-lib linkage as FrameworkGame so the SFMX_*_EXPORT macros resolve to
# nothing; flip this when the module becomes a DLL.
target_compile_definitions(ImageWebP PUBLIC SFMX_STATIC_LIB)

# FrameworkGame PUBLIC: the module's public API leans on the core (IDecoder,
# AssetManager, sf::Image). libwebp PRIVATE: an internal detail, not leaked.
target_link_libraries(ImageWebP
    PUBLIC  SFMX::FrameworkGame
    PRIVATE webpdecoder)
