add_executable(UnitTests
    UnitTest.cpp
    FlagTest.cpp
    DataStreamTest.cpp
    SpriteAtlasTest.cpp
    FileSystemTest.cpp
)

target_link_libraries(UnitTests PRIVATE SFMX::FrameworkGame)

# Each test group lives in its own .cpp here; UnitTest.cpp is the runner.
