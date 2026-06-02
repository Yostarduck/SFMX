#pragma once

/************************************************************************/
/**
 * Initial platform/compiler-related stuff to set.
*/
/************************************************************************/
#include <cassert>
#include "Using.h"

// Define the actual endian type (little endian for Windows, Linux, Apple and PS4)
//TODO not in use yet, but we should eventually use this to detect endianness instead of relying on platform defines
#define SFMX_ENDIAN_LITTLE                   IN_USE
#define SFMX_ENDIAN_BIG                      NOT_IN_USE

#define SFMX_VERSION_MAJOR    0
#define SFMX_VERSION_MINOR    2
#define SFMX_VERSION_PATCH    0
#define SFMX_VERSION_BUILD    1

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// The version string macro - completely resolved at compile time
#define SFMX_ENGINE_VERSION_STRING \
    TOSTRING(SFMX_VERSION_MAJOR) "." \
    TOSTRING(SFMX_VERSION_MINOR) "." \
    TOSTRING(SFMX_VERSION_PATCH) "." \
    TOSTRING(SFMX_VERSION_BUILD)

/************************************************************************/
/**
 * Compiler type and version
 */
 /************************************************************************/
#if defined(__clang__)
#   define SFMX_COMPILER_MSVC               NOT_IN_USE
#   define SFMX_COMPILER_GNUC               NOT_IN_USE
#   define SFMX_COMPILER_INTEL              NOT_IN_USE
#   define SFMX_COMPILER_CLANG              IN_USE
#   define SFMX_COMP_VER __clang_version__
#   define SFMX_THREADLOCAL __thread
#   define SFMX_STDCALL __attribute__((stdcall))
#   define SFMX_CDECL __attribute__((cdecl))
#   define SFMX_FALLTHROUGH [[clang::fallthrough]];
#elif defined (__GNUC__) // Check after Clang, as Clang defines this too
#   define SFMX_COMPILER_MSVC               NOT_IN_USE
#   define SFMX_COMPILER_GNUC               IN_USE
#   define SFMX_COMPILER_INTEL              NOT_IN_USE
#   define SFMX_COMPILER_CLANG              NOT_IN_USE
#   define SFMX_COMP_VER (((__GNUC__)*100)+(__GNUC_MINOR__*10)+__GNUC_PATCHLEVEL__)
#   define SFMX_THREADLOCAL __thread
#   define SFMX_STDCALL __attribute__((stdcall))
#   define SFMX_CDECL __attribute__((cdecl))
#   define SFMX_FALLTHROUGH __attribute__((fallthrough));
#elif defined (__INTEL_COMPILER)
#   define SFMX_COMPILER_MSVC               NOT_IN_USE
#   define SFMX_COMPILER_GNUC               NOT_IN_USE
#   define SFMX_COMPILER_INTEL              IN_USE
#   define SFMX_COMPILER_CLANG              NOT_IN_USE
#   define SFMX_COMP_VER __INTEL_COMPILER
#   define SFMX_STDCALL __stdcall
#   define SFMX_CDECL __cdecl
#   define SFMX_FALLTHROUGH

// SFMX_THREADLOCAL define is down below because Intel compiler defines it
// differently based on platform

// Check after Clang and Intel, we could be building with either with VS
#elif defined (_MSC_VER)
#   define SFMX_COMPILER_MSVC               IN_USE
#   define SFMX_COMPILER_GNUC               NOT_IN_USE
#   define SFMX_COMPILER_INTEL              NOT_IN_USE
#   define SFMX_COMPILER_CLANG              NOT_IN_USE
#   define SFMX_COMP_VER                    _MSC_VER
#   define SFMX_THREADLOCAL                 __declspec(thread)
#   define SFMX_STDCALL                     __stdcall
#   define SFMX_CDECL                       __cdecl
#   define SFMX_FALLTHROUGH
#   undef                                   __PRETTY_FUNCTION__
#   define                                  __PRETTY_FUNCTION__ __FUNCSIG__
#else
// No known compiler found, send the error to the output (if any)
#   define SFMX_COMPILER_MSVC               NOT_IN_USE
#   define SFMX_COMPILER_GNUC               NOT_IN_USE
#   define SFMX_COMPILER_INTEL              NOT_IN_USE
#   define SFMX_COMPILER_CLANG              NOT_IN_USE
#   define SFMX_COMP_VER                    0
#   pragma error "No known compiler."
#endif

#define SFMX_PARAMETER_UNUSED(x) (void)x

/************************************************************************/
/**
 * See if we can use __forceinline or if we need to use __inline instead
 */
 /************************************************************************/
#if USING(SFMX_COMPILER_MSVC)
#  define SFMX_CPP17_OR_LATER              USE_IF(_MSVC_LANG >= 201703L)
#  define SFMX_CPP20_OR_LATER              USE_IF(_MSVC_LANG >= 202002L)
# if SFMX_COMP_VER >= 1920
#  define NODISCARD [[nodiscard]]
# else
#  define NODISCARD
#  define SFMX_CPP17_OR_LATER              USE_IF(__cplusplus >= 201703L)
#  define SFMX_CPP20_OR_LATER              USE_IF(__cplusplus >= 202002L)
# endif
# if SFMX_COMP_VER >= 1200
#   define FORCEINLINE                     __forceinline
#   ifndef RESTRICT
#     define RESTRICT                      __restrict
#   endif
# endif
#elif defined (__MINGW32__)
# if !defined (FORCEINLINE)
#   define FORCEINLINE                     __inline
#   ifndef RESTRICT
#     define RESTRICT
#   endif
# endif
#else
#  define SFMX_CPP17_OR_LATER              USE_IF(__cplusplus >= 201703L) // CPP 17
#  define SFMX_CPP20_OR_LATER              USE_IF(__cplusplus >= 202002L) // CPP 20
#  define NODISCARD [[nodiscard]]
#  define FORCEINLINE                      __inline
#  ifndef RESTRICT
#    define RESTRICT                       __restrict
#  endif
#endif

/************************************************************************/
/**
 * Finds the current platform
 */
 /************************************************************************/
#if defined(_WIN32) || defined(_WIN64)
#  define SFMX_PLATFORM_WINDOWS            1
#  define SFMX_PLATFORM_MACOS              0
#  define SFMX_PLATFORM_LINUX              0
#elif defined(__APPLE__)
#  define SFMX_PLATFORM_WINDOWS            0
#  define SFMX_PLATFORM_MACOS              1
#  define SFMX_PLATFORM_LINUX              0
#elif defined(__linux__)
#  define SFMX_PLATFORM_WINDOWS            0
#  define SFMX_PLATFORM_MACOS              0
#  define SFMX_PLATFORM_LINUX              1
#else
#  define SFMX_PLATFORM_WINDOWS            0
#  define SFMX_PLATFORM_MACOS              0
#  define SFMX_PLATFORM_LINUX              0
#endif

// Convert to IN_USE/NOT_IN_USE format for compatibility with existing system
#if SFMX_PLATFORM_WINDOWS
#  define SFMX_PLATFORM_WIN32              IN_USE
#  define SFMX_PLATFORM_OSX                NOT_IN_USE
#  define SFMX_PLATFORM_LINUX_COMPAT       NOT_IN_USE
#elif SFMX_PLATFORM_MACOS
#  define SFMX_PLATFORM_WIN32              NOT_IN_USE
#  define SFMX_PLATFORM_OSX                IN_USE
#  define SFMX_PLATFORM_LINUX_COMPAT       NOT_IN_USE
#elif SFMX_PLATFORM_LINUX
#  define SFMX_PLATFORM_WIN32              NOT_IN_USE
#  define SFMX_PLATFORM_OSX                NOT_IN_USE
#  define SFMX_PLATFORM_LINUX_COMPAT       IN_USE
#else
#  define SFMX_PLATFORM_WIN32              NOT_IN_USE
#  define SFMX_PLATFORM_OSX                NOT_IN_USE
#  define SFMX_PLATFORM_LINUX_COMPAT       NOT_IN_USE
#endif

/************************************************************************/
/**
 * Find the architecture type
 */
 /************************************************************************/
#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__) || defined(_M_ARM64)
#  define SFMX_PLATFORM_64BIT              1
#  define SFMX_PLATFORM_32BIT              0
#  define SFMX_ARCHITECTURE_X86_64         IN_USE
#  define SFMX_ARCHITECTURE_X86_32         NOT_IN_USE
#else
#  define SFMX_PLATFORM_64BIT              0
#  define SFMX_PLATFORM_32BIT              1
#  define SFMX_ARCHITECTURE_X86_64         NOT_IN_USE
#  define SFMX_ARCHITECTURE_X86_32         IN_USE
#endif

/************************************************************************/
/**
 * Memory Alignment macros
 */
 /************************************************************************/
#if USING(SFMX_COMPILER_MSVC)
# define MS_ALIGN(n)                        __declspec(align(n))
# ifndef GCC_PACK
#   define GCC_PACK(n)
# endif
# ifndef GCC_ALIGN
#   define GCC_ALIGN(n)
# endif
#elif USING(SFMX_COMPILER_GNUC)
# define MS_ALIGN(n)
# define GCC_PACK(n)
# define GCC_ALIGN(n)                      __attribute__((__aligned__(n)))
#else
# define MS_ALIGN(n)
# define GCC_PACK(n)                       __attribute__((packed, aligned(n)))
# define GCC_ALIGN(n)                      __attribute__((__aligned__(n)))
#endif

#if defined(__GNUC__) || defined(__clang__)
#define DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define DEPRECATED __declspec(deprecated)
#else
#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#define DEPRECATED
#endif

/************************************************************************/
/**
 * For noexcept override
 */
 /************************************************************************/
#if USING(SFMX_COMPILER_MSVC)
# define SFMX_NOEXCEPT                        noexcept
#elif USING(SFMX_COMPILER_INTEL)
# define SFMX_NOEXCEPT                        noexcept
#elif USING(SFMX_COMPILER_GNUC)
# define SFMX_NOEXCEPT                        noexcept
#else
# define SFMX_NOEXCEPT
#endif

/************************************************************************/
/**
 * Library export specifics
 */
 /************************************************************************/
#if USING(SFMX_PLATFORM_WIN32)
# if USING(SFMX_COMPILER_MSVC)
#   if defined( SFMX_STATIC_LIB )
#     define SFMX_UTILITY_EXPORT
#   else
#     if defined ( SFMX_UTILITY_EXPORTS )
#       define SFMX_UTILITY_EXPORT          __declspec( dllexport )
#     else
#       define SFMX_UTILITY_EXPORT          __declspec( dllimport )
#     endif
#   endif
# else // Any other compiler
#   if defined( SFMX_STATIC_LIB )
#     define SFMX_UTILITY_EXPORT
#   else
#     if defined( SFMX_UTILITY_EXPORTS )
#       define SFMX_UTILITY_EXPORT          __attribute__ ((dllexport))
#     else
#       define SFMX_UTILITY_EXPORT          __attribute__ ((dllimport))
#     endif
#   endif
# endif
# define SFMX_UTILITY_HIDDEN
#else
# if defined( SFMX_UTILITY_EXPORTS )
#   define SFMX_UTILITY_EXPORT              __attribute__ ((visibility ("default")))
# else
#   define SFMX_UTILITY_EXPORT
# endif
# define SFMX_UTILITY_HIDDEN                __attribute__ ((visibility ("hidden")))
#endif

// DLL export plugins
#if USING(SFMX_PLATFORM_WIN32)
# if USING(SFMX_COMPILER_MSVC)
#   define SFMX_PLUGIN_EXPORT               __declspec(dllexport)
# else
#   define SFMX_PLUGIN_EXPORT               __attribute__ ((dllexport))
# endif
#else
#  define SFMX_PLUGIN_EXPORT                __attribute__((visibility("default")))
#endif

/************************************************************************/
/**
 * Platform specific Settings
 */
 /************************************************************************/
// Windows
#if USING(SFMX_PLATFORM_WIN32)
# if defined(_DEBUG) || defined(DEBUG)
#   define SFMX_DEBUG_MODE                  IN_USE
# else
#   define SFMX_DEBUG_MODE                  NOT_IN_USE
# endif
# if USING(SFMX_COMPILER_INTEL)
#   define SFMX_THREADLOCAL                 __declspec(thread)
# endif
#endif

// Linux & macOS
#if USING(SFMX_PLATFORM_LINUX_COMPAT) || USING(SFMX_PLATFORM_OSX)
# if defined(_DEBUG) || defined(DEBUG)
#   define SFMX_DEBUG_MODE                  IN_USE
# else
#   define SFMX_DEBUG_MODE                  NOT_IN_USE
# endif
# if USING(SFMX_COMPILER_INTEL)
#   define SFMX_THREADLOCAL                 thread
# endif
#endif

/************************************************************************/
/**
 * Definition of Debug macros
 */
 /************************************************************************/
#if USING(SFMX_DEBUG_MODE)
# define SFMX_ASSERT(x)                    assert(x)
# define SFMX_DEBUG_ONLY(x)                x
#else
# define SFMX_DEBUG_ONLY(x)
# define SFMX_ASSERT(x)
#endif

#define SFMX_ENABLE_BACKTRACE USE_IF(USING(SFMX_DEBUG_MODE)))

/************************************************************************/
/*
 * Extern.
 */
 /************************************************************************/
#define SFMX_EXTERN extern "C"

/************************************************************************/
/**
 * Disable some compiler warnings
 */
 /************************************************************************/

// If we are compiling with Visual Studio
#if USING(SFMX_COMPILER_MSVC)
  // Secure versions aren't multi platform, so we won't be using them
# define _CRT_SECURE_NO_WARNINGS

  // Disable: nonstandard extension used: nameless struct/union.
# pragma warning(disable : 4201)

  // Disable: "<type> needs to have DLL interface to be used by clients'
# pragma warning(disable : 4251)

  // Disable: 'X' Function call with parameters that may be unsafe
# pragma warning(disable : 4996)

  // Disable: decorated name length exceeded, name was truncated.
# pragma warning(disable : 4503)
#endif