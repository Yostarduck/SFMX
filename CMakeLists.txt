cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(SFMX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(_arch_hint "")

if(CMAKE_GENERATOR_PLATFORM)
  string(TOLOWER "${CMAKE_GENERATOR_PLATFORM}" _arch_hint)
else()
  string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _arch_hint)
endif()

if(_arch_hint MATCHES "^(x64|amd64|x86_64)$")
  set(ARCH_DIR "x64")
elseif(_arch_hint MATCHES "^(win32|x86|i[3-6]86)$")
  set(ARCH_DIR "x86")
elseif(_arch_hint MATCHES "^(arm64|aarch64)$")
  set(ARCH_DIR "arm64")
elseif(_arch_hint MATCHES "^arm(v[0-9]+)?")
  set(ARCH_DIR "arm")
else()
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_DIR "x64")
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(ARCH_DIR "x86")
  else()
    message(FATAL_ERROR "Couldn't determine target architecture.")
  endif()
endif()

message(STATUS "Target architecture detected: hint='${_arch_hint}', ARCH_DIR='${ARCH_DIR}'")

# Normalize common config names to "Capitalized" form
set(CONFIG_DIR "$<IF:$<CONFIG:Debug>,Debug,$<IF:$<CONFIG:Release>,Release,$<IF:$<CONFIG:RelWithDebInfo>,RelWithDebInfo,$<IF:$<CONFIG:MinSizeRel>,MinSizeRel,$<CONFIG>>>>>")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${ARCH_DIR}/${CONFIG_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${ARCH_DIR}/${CONFIG_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${ARCH_DIR}/${CONFIG_DIR})

foreach(OUTPUTCONFIG Debug Release RelWithDebInfo MinSizeRel)
    foreach(OUTPUTTYPE RUNTIME LIBRARY ARCHIVE)
        set(CMAKE_${OUTPUTTYPE}_OUTPUT_DIRECTORY_${OUTPUTCONFIG}
            ${CMAKE_BINARY_DIR}/${ARCH_DIR}/${OUTPUTCONFIG})
    endforeach()
endforeach()

add_compile_definitions($<$<CONFIG:Debug>:DEBUG>
                        $<$<CONFIG:Release>:RELEASE>)

enable_testing()

add_subdirectory(SketchGfx)
add_subdirectory(FrameworkGame)
add_subdirectory(Game)
add_subdirectory(Tests)
