#include "Game.h"
#include "core/ArgumentParser.h"
namespace sfmx
{

Game::Game()  {

}

Game::~Game()  {
}

void Game::initialize(int argc, char **argv)  {
  // Implementation of initialize
  ArgumentParser argParser;
  argParser.parse(argc, argv);
  if (argParser.hasFlag("cook")) {
    cook();
  }

  if (argParser.hasFlag("cook-scene")) {
    cookGame();
  }

  if (argParser.hasFlag("content-dir")) {

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

}

void Game::cookGame() {
  
}

}