#include "App.h"

namespace sfmx
{


class Game : public App {
 public:
  
  Game();

  virtual ~Game();

  virtual void
  initialize(int argc, char **argv) override;

  virtual void
  run() override;

  virtual void
  shutdown() override;

  void
  cook();

  void
  cookGame();

 private:


};

}