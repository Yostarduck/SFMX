add_executable(UnitTests
    UnitTest.cpp
    FlagTest.cpp
    DataStreamTest.cpp
    ParticleSystemTest.cpp
    BenchmarkTest.cpp
    SpriteAtlasTest.cpp
    FileSystemTest.cpp
)

target_include_directories(UnitTests PRIVATE
    ${CMAKE_SOURCE_DIR}/ThirdParty/doctest
    ${CMAKE_SOURCE_DIR}/ThirdParty/nanobench
)

target_link_libraries(UnitTests PRIVATE SFMX::FrameworkGame)

add_test(NAME UnitTests COMMAND UnitTests)

# Each test group lives in its own .cpp here; UnitTest.cpp is the runner.
