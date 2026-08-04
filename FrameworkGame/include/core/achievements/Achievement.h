#pragma once

#include "core/platform/Prerequisites.h"
#include "utils/UUID.h"
#include "assets/TextureAsset.h"
#include "utils/Module.h"
#include "utils/EventSystem.h"

namespace sfmx
{

  
  struct Achievement
  {
    UUID    id;
    String  name;
    String  description;
    bool    unlocked;
    SPtr<TextureAsset> icon;
  };

// This is the list of all achievements in the game. 
// We should load the data, and check against the user data to see what is actually unlocked or not
static Array<Achievement, 10> kAchievements = {{
  { UUID::createFromName("tutorial"), "First Steps",    "Complete the tutorial.",             false, nullptr },
  { UUID::createFromName("firebegin"), "Flame on",    "Master all fire element upgrades.",  false, nullptr },
  { UUID::createFromName("firemaster"), "Fast and Fieryous",    "Master all fire element upgrades.",  false, nullptr },
  { UUID::createFromName("airbegin"), "Windy out here",    "Master all fire element upgrades.",  false, nullptr },
  { UUID::createFromName("airmaster"), "Blasted away",    "Master all fire element upgrades.",  false, nullptr },
  { UUID::createFromName("zapbegin"), "Watts up",    "Master all fire element upgrades.",  false, nullptr },
  { UUID::createFromName("zapmaster"), "",    "Master all fire element upgrades.",  false, nullptr },
  { UUID::createFromName("icebegin"), "Flake out",    "Master all fire element upgrades.",  false, nullptr },
  { UUID::createFromName("bombbegin"), "Explooosion",    "Master all fire element upgrades.",  false, nullptr },
  { UUID::createFromName("voidbegin"), "Void Master",    "Master all fire element upgrades.",  false, nullptr }
}};

using AchievementEvent = void(const Achievement&);

class AchievementManager : Module<AchievementManager>
{
  
  void
  loadUserStats();

  void
  loadAchievements();

  void
  unlockAchievement(const UUID& id);

  // Anyone can subscribe into this event to be notified
  Event<AchievementEvent> mutable m_onAchievementUnlocked;

  UnorderedMap<UUID, Achievement> m_userAchievements;

};

}