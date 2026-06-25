add_executable(Game main.cpp)

target_link_libraries(Game PRIVATE SFMX::FrameworkGame)

# Auto-cook the media after building: wraps Game/resources/*.png into the cooked
# Game/assets/*.sfmxasset the AssetManager mounts at runtime. Keeps a fresh clone
# working with just "build + run" (no manual cook). Headless: --cook returns
# before any window/asset load.
add_custom_command(TARGET Game POST_BUILD
    COMMAND $<TARGET_FILE:Game> --cook
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VERBATIM
    COMMENT "Cooking assets: Game/resources -> Game/assets")