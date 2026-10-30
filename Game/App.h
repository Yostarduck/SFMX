#include "core/platform/Prerequisites.h"
#include "core/platform/STDHeaders.h"
#include "render/PostProcessPipeline.h"
#include "config/IniFile.h"


namespace sfmx
{

class App {
 public:
  App() = default;
  virtual ~App() = default;

  virtual void
  initialize(int argc, char **argv) = 0;

  virtual void  
  pollEvents() = 0;

  virtual void
  update(float deltaTime) = 0;

  virtual void 
  render() = 0;

  virtual void
  run() = 0;

  virtual void
  shutdown() = 0;

  bool
  shouldExit() const { return m_shouldExit; }

  int 
  exitCode() const { return m_exitCode; }

 protected:
  IniFile m_config;
  PostProcessPipeline m_postProcessPipeline;
  bool m_shouldExit = false;
  int m_exitCode = 0;
};

}
