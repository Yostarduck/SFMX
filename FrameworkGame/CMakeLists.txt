add_library(FrameworkGame STATIC
    source/config/IniFile.cpp
    source/core/MemoryPool.cpp
)
add_library(SFMX::FrameworkGame ALIAS FrameworkGame)

target_include_directories(FrameworkGame
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/ThirdParty/stdUUID
    PRIVATE
        ${CMAKE_SOURCE_DIR}/ThirdParty/mINI
)

target_link_libraries(FrameworkGame PUBLIC SFMX::SketchGfx)
