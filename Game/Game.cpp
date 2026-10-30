#include "Game.h"

#include "assets/AssetCooker.h"
#include "assets/AssetImporterRegistry.h"
#include "ImageWebP.h" // format module: self-registers WebP decoder + import rule

namespace sfmx
{

Game::Game()  {

}

Game::~Game()  {
}

void Game::initialize(int argc, char **argv)  {
  // Implementation of initialize
  
  m_argParser.parse(argc, argv);
  if (m_argParser.hasFlag("cook")) {
    cook();
  }

  if (m_argParser.hasFlag("cook-scene")) {
    cookGame();
  }

  if (m_argParser.hasFlag("content-dir")) {

  }



}

void
Game::pollEvents() {

}

void 
Game::update(float deltaTime) {

}

void
Game::render() {

}

void Game::run() {
  // Game loop
}


void Game::shutdown()  {
  // Clean up everything

}

void Game::cook() {
  auto& flagValues = m_argParser.getFlagValues("cook");
  const FileSystemPath srcDir = flagValues.size() > 0 ? flagValues[0] : "Game/resources";
  const FileSystemPath outDir = flagValues.size() > 1 ? flagValues[1] : "Game/assets";
  AssetImporterRegistry::startUp();
  AssetImporterRegistry::instance().registerBuiltins();
    
  imagewebp::registerModule(); // adds the .webp import rule (decoder skipped: no AssetManager in cook)
  AssetCooker::cookDirectory(srcDir, outDir);
  AssetImporterRegistry::shutDown();
}

void Game::cookGame() {
  
}

}