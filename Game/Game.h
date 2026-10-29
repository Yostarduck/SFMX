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

  bool
  shouldExit() const { return m_exitCode != 0; }

  int 
  exitCode() const { return m_exitCode; }

 private:

 int m_exitCode;

};

}