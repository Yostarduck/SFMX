#include "App.h"

#include "core/ArgumentParser.h"

namespace sfmx
{


class Game : public App {
 public:
  
  Game();

  virtual ~Game();

  virtual void
  initialize(int argc, char **argv) override;

  virtual void  
  pollEvents() override;

  virtual void
  update(float deltaTime) override;

  virtual void 
  render() override;

  virtual void
  run() override;

  virtual void
  shutdown() override;

  bool
  cook();

  bool
  cookGame();


 private:
  ArgumentParser m_argParser;
  IniFile m_gameConfig;

};

}