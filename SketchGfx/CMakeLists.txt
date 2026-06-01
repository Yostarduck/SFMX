if(WIN32)
  # ...
elseif(APPLE)
  # ...
elseif(UNIX)
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "Unsupported UNIX platform: ${CMAKE_SYSTEM_NAME}")
  endif()

  set(SFW_USE_XCB     ON  CACHE BOOL "Use XCB backend on Linux")
  set(SFW_USE_WAYLAND OFF CACHE BOOL "Use Wayland backend on Linux")
  set(SFW_USE_X11     OFF CACHE BOOL "Use X11 backend on Linux")

  set(_linux_backend_count 0)
  foreach(_backend SFW_USE_XCB SFW_USE_WAYLAND SFW_USE_X11)
    if(${_backend})
      math(EXPR _linux_backend_count "${_linux_backend_count} + 1")
    endif()
  endforeach()

  if(_linux_backend_count GREATER 1)
    message(FATAL_ERROR "Only one Linux backend can be enabled: SFW_USE_XCB, SFW_USE_WAYLAND, SFW_USE_X11.")
  elseif(_linux_backend_count EQUAL 0)
    set(SFW_USE_XCB ON CACHE BOOL "Use XCB backend on Linux" FORCE)
  endif()
else()
  message(FATAL_ERROR "Unsupported platform")
endif()

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

add_library(SketchGfx INTERFACE)
add_library(SFMX::SketchGfx ALIAS SketchGfx)

target_link_libraries(SketchGfx INTERFACE SFML::Graphics)

