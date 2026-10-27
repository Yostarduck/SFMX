#include "Game.h"
#include "core/ArgumentParser.h"
namespace sfmx
{

Game::Game() 
{

}

Game::~Game() 
{
}

void Game::initialize(int argc, char **argv) 
{
  // Implementation of initialize
  ArgumentParser argParser;
  argParser.parse(argc, argv);

  

}

void Game::run()
{
  // Game loop
}


void Game::shutdown() 
{
  // Clean up everything

}

}