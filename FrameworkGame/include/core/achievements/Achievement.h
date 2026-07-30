#pragma once

#include "STDHeaders.h"
#include "UUID.h"
#include "assets/TextureAsset.h"
#include "utils/Module.h"
#include "utils/Event.h"

namespace sfmx
{
enum EAchievements
{
  
}

struct Achievement
{
  UUID    id;
  String  name;
  String  description;
  bool    unlocked;
  SPtr<TextureAsset> icon;
};

class AchievementManager : Module<AchievementManager>
{
  
  void
  loadUserStats();

  void
  loadAchievements();


  // Anyone can subscribe into this event to be notified
  Event<void, const Achievement&> onAchievementUnlocked;



};

}