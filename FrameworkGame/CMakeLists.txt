add_library(FrameworkGame STATIC
    source/config/IniFile.cpp
    source/core/MemoryPool.cpp
    source/scene/Component.cpp
    source/scene/Transform.cpp
    source/scene/SceneNode.cpp
    source/scene/Scene.cpp
)
add_library(SFMX::FrameworkGame ALIAS FrameworkGame)

target_include_directories(FrameworkGame
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/ThirdParty/mINI
        ${CMAKE_SOURCE_DIR}/ThirdParty/stdUUID
)

target_link_libraries(FrameworkGame PUBLIC SFMX::SketchGfx)
