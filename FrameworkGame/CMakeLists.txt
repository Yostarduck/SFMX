add_library(FrameworkGame STATIC source/test.cpp)
add_library(SFMX::FrameworkGame ALIAS FrameworkGame)

target_include_directories(FrameworkGame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(FrameworkGame PUBLIC SFMX::SketchGfx)
