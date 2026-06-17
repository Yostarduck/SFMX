add_library(FrameworkGame STATIC
    source/config/IniFile.cpp
    source/core/MemoryPool.cpp
    source/core/DataStream.cpp
    source/core/MemoryDataStream.cpp
    source/core/FileDataStream.cpp
    source/core/FileSystem.cpp
    source/assets/AssetFile.cpp
    source/input/InputTypes.cpp
    source/input/Keyboard.cpp
    source/input/Mouse.cpp
    source/input/Gamepad.cpp
    source/input/Processor.cpp
    source/input/Interaction.cpp
    source/input/InputControl.cpp
    source/input/InputAction.cpp
    source/input/ActionMap.cpp
    source/input/Mapping.cpp
    source/input/InputSystem.cpp
    source/resource/SpriteAtlas.cpp
    source/scene/Component.cpp
    source/scene/Transform.cpp
    source/scene/ParticleSystemComponent.cpp
    source/scene/SceneNode.cpp
    source/scene/Scene.cpp
    source/scene/SourceComponent.cpp
    source/scene/ListenerComponent.cpp
    source/scene/CameraComponent.cpp
    source/scene/SpriteComponent.cpp
    source/scene/AnimatorComponent.cpp
    source/utils/MemoryPoolHandler.cpp
    source/utils/Random.cpp
    source/utils/UUID.cpp
)
add_library(SFMX::FrameworkGame ALIAS FrameworkGame)

target_include_directories(FrameworkGame
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/ThirdParty/stdUUID
    PRIVATE
        ${CMAKE_SOURCE_DIR}/ThirdParty/mINI
)

# FrameworkGame is a static library, so the SFMX_UTILITY_EXPORT family must
# resolve to nothing (not dllimport/dllexport). PUBLIC so consumers link with
# matching, consistent symbol linkage.
target_compile_definitions(FrameworkGame PUBLIC SFMX_STATIC_LIB)

target_link_libraries(FrameworkGame PUBLIC SFMX::SketchGfx)
