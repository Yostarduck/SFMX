#include "core/achievements/Achievement.h"

namespace sfmx
{
void AchievementManager::loadUserStats()
{
  
}

void AchievementManager::loadAchievements()
{
  m_userAchievements.reserve(kAchievements.size());
  for(auto& achievement : kAchievements)
  {
    m_userAchievements.try_emplace(achievement.id, achievement);
  }
}

void AchievementManager::unlockAchievement(const UUID& id)
{
  if (m_userAchievements.contains(id))
  {
    auto& achievement = m_userAchievements[id];
    // Unlock the achievement
    if (!achievement.unlocked)
    {
      // Notify subscribers
      achievement.unlocked = true;
      m_onAchievementUnlocked(achievement);
    }
  }
}



} // namespace sfmx
