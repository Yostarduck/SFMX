add_executable(UnitTests
    UnitTest.cpp
    FlagTest.cpp
    DataStreamTest.cpp
    ParticleSystemTest.cpp
    BenchmarkTest.cpp
    SpriteAtlasTest.cpp
    FileSystemTest.cpp
    CollisionTest.cpp
    AssetFileTest.cpp
    AssetCodecTest.cpp
    LuaAssetTest.cpp
    AssetManagerTest.cpp
    AsyncLoadTest.cpp
    ComponentRegistryTest.cpp
    SceneSerializerTest.cpp
    ComponentSerializationTest.cpp
    SpriteComponentSerializationTest.cpp
    ScriptComponentSerializationTest.cpp
    SourceComponentSerializationTest.cpp
    UIButtonSerializationTest.cpp
    AnimatorComponentSerializationTest.cpp
    ParticleSystemComponentSerializationTest.cpp
    AssetCookerTest.cpp
    ImageDecoderSeamTest.cpp
    ImageWebPModuleTest.cpp
    LocalizationAssetTest.cpp
    ScriptHotReloadTest.cpp
)

target_include_directories(UnitTests PRIVATE
    ${CMAKE_SOURCE_DIR}/ThirdParty/doctest
    ${CMAKE_SOURCE_DIR}/ThirdParty/nanobench
)

# SFMX::ImageWebP: the WebP format module, exercised by ImageWebPModuleTest.cpp. It
# links libwebp PRIVATE, so UnitTests gets WebP decode transitively without the core
# ever referencing libwebp.
target_link_libraries(UnitTests PRIVATE SFMX::FrameworkGame SFMX::ImageWebP)

add_test(NAME UnitTests COMMAND UnitTests)

# Each test group lives in its own .cpp here; UnitTest.cpp is the runner.
