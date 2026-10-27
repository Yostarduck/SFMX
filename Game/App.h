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
  run() = 0;

  virtual void
  shutdown() = 0;

 protected:
  IniFile m_config;
  PostProcessPipeline m_postProcessPipeline;

};

}
