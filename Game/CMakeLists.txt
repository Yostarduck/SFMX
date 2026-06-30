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
# implicit GL context (atlas frame detection).
add_custom_command(TARGET Game POST_BUILD
    COMMAND $<TARGET_FILE:Game> --cook
    COMMAND $<TARGET_FILE:Game> --cook-scene
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VERBATIM
    COMMENT "Cooking: media (Game/resources -> Game/assets), then the demo scene")