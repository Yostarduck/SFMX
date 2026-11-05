#pragma once
#include "core/platform/Prerequisites.h"

namespace sfmx {


  class Scene;
  class SceneNode;
  class SourceComponent;
  class MemoryPoolHandler;


class Cooker
{
 public:
  Cooker() = default;
  ~Cooker() = default;

  void cookGameData();


  static const StringView kCookingDataFIle;

  static void
  registerPools(MemoryPoolHandler& pools);

  static void
  poolsInfo();

  static void 
  registerComponents();

  static bool
  buildGameData();


};

}

