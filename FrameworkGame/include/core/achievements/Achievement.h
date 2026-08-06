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