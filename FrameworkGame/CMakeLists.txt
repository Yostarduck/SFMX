include(FetchContent)

FetchContent_Declare(luajit_src
    GIT_REPOSITORY https://github.com/LuaJIT/LuaJIT.git
    GIT_TAG b925b3e3fc6771171602323b45fbe9fb8fc90369
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(luajit_src)

set(LUAJIT_DIR "${luajit_src_SOURCE_DIR}" CACHE PATH "" FORCE)
set(LUAJIT_BUILD_EXE OFF CACHE BOOL "" FORCE)
FetchContent_Declare(luajit_cmake
    GIT_REPOSITORY https://github.com/zhaozg/luajit-cmake.git
    GIT_TAG bdc1b54d0572e2cc5c3eaa5c1e818d0e0246b9d1
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(luajit_cmake)

FetchContent_Declare(sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG v3.5.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(sol2)

add_library(FrameworkGame STATIC
    source/config/IniFile.cpp

    source/core/MemoryPool.cpp
    source/core/DataStream.cpp
    source/core/MemoryDataStream.cpp
    source/core/FileDataStream.cpp
    source/core/FileSystem.cpp
    source/core/physics/Collider.cpp
    source/core/physics/PhysicsSystem.cpp

    source/assets/AssetFile.cpp
    source/assets/AssetCodecRegistry.cpp
    source/assets/TextureAsset.cpp
    source/assets/TextureCodec.cpp

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

    source/scene/Scene.cpp
    source/scene/SceneNode.cpp
    source/scene/Component.cpp
    source/scene/Transform.cpp
    source/scene/CameraComponent.cpp
    source/scene/SourceComponent.cpp
    source/scene/ListenerComponent.cpp
    source/scene/SpriteComponent.cpp
    source/scene/AnimatorComponent.cpp
    source/scene/ColliderComponent.cpp
    source/scene/RigidBodyComponent.cpp
    source/scene/ParticleSystemComponent.cpp
    source/scene/ScriptComponent.cpp

    source/utils/MemoryPoolHandler.cpp
    source/utils/Random.cpp
    source/utils/UUID.cpp

    source/scripts/ScriptEngine.cpp
    source/scripts/RegisterAngle.cpp
    source/scripts/RegisterColor.cpp
    source/scripts/RegisterVector2i.cpp
    source/scripts/RegisterVector2f.cpp
    source/scripts/RegisterVector3i.cpp
    source/scripts/RegisterVector3f.cpp
    source/scripts/RegisterIntRect.cpp
    source/scripts/RegisterFloatRect.cpp
    source/scripts/RegisterTransform.cpp
    source/scripts/RegisterInputTypes.cpp
    source/scripts/RegisterKeyboard.cpp
    source/scripts/RegisterMouse.cpp
    source/scripts/RegisterGamepad.cpp
    source/scripts/RegisterComponent.cpp
    source/scripts/RegisterComponentAccess.cpp
    source/scripts/RegisterTransformComponent.cpp
    source/scripts/RegisterScene.cpp
    source/scripts/RegisterSceneNode.cpp
    source/scripts/RegisterAll.cpp)

add_library(SFMX::FrameworkGame ALIAS FrameworkGame)

target_include_directories(FrameworkGame
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/ThirdParty/stdUUID
        ${luajit_cmake_BINARY_DIR}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/ThirdParty/mINI)

# FrameworkGame is a static library, so the SFMX_UTILITY_EXPORT family must
# resolve to nothing (not dllimport/dllexport). PUBLIC so consumers link with
# matching, consistent symbol linkage.
target_compile_definitions(FrameworkGame PUBLIC
    SFMX_STATIC_LIB
    SOL_LUAJIT=1)

target_link_libraries(FrameworkGame PUBLIC
    SFMX::SketchGfx
    luajit::lib
    luajit::header
    sol2::sol2)
