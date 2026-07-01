add_executable(Game
    main.cpp
    DemoScene.cpp
    DemoCook.cpp)

target_link_libraries(Game PRIVATE SFMX::FrameworkGame)

# Auto-cook after building so a fresh clone works with just "build + run":
#   1. --cook       wraps Game/resources/* media into Game/assets/*.sfmxasset.
#   2. --cook-scene builds the demo scene in code and serializes it to
#      Game/assets/demo.sfmxasset (which the run then loads).
# Order matters: the scene cook resolves textures by UUID, so the media must be
# cooked first. --cook is headless; --cook-scene is windowless but uses SFML's
# implicit GL context (atlas frame detection). Both run from the repo root.
#
# Then stage the RUNTIME content next to the exe, so the runtime — which resolves
# content relative to the executable (FileSystem content root) — finds it in a dev
# build exactly like a shipped one, no matter the launch CWD. We stage:
#   - assets/  : the cooked .sfmxasset (textures/scene loaded by UUID).
#   - config/  : the .ini files.
#   - resources/: ONLY the loose files the runtime still reads by path — just the
#     mp3 music (streams, not cooked). We deliberately do NOT copy any cooked SOURCE
#     (png/jpg/bmp/ogg/wav/flac, and now .lua → LuaAsset) — those already ship cooked
#     in assets/, loaded by UUID, so staging the sources would be redundant.
file(GLOB SFMX_LOOSE_RUNTIME CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/Game/resources/*.mp3)

add_custom_command(TARGET Game POST_BUILD
    COMMAND $<TARGET_FILE:Game> --cook
    COMMAND $<TARGET_FILE:Game> --cook-scene
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            ${CMAKE_SOURCE_DIR}/Game/assets $<TARGET_FILE_DIR:Game>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            ${CMAKE_SOURCE_DIR}/Game/config $<TARGET_FILE_DIR:Game>/config
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:Game>/resources
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SFMX_LOOSE_RUNTIME} $<TARGET_FILE_DIR:Game>/resources
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VERBATIM
    COMMENT "Cook (media + scene) then stage assets/config + loose runtime (mp3/lua)")