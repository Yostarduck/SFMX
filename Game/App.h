
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

};

}
