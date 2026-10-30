#include <exception>
#include <iostream>

#include "Game.h"

int
main(int argc, char **argv) {
  try {
    sfmx::Game game;
    game.initialize(argc, argv);
    if (!game.shouldExit()) {
      game.run();
    }
    game.shutdown();
    return game.exitCode();
  }
  catch (const std::exception &error) {
    std::cerr << "[Fatal] " << error.what() << '\n';
    return 1;
  }
}
