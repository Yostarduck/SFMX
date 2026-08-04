#include "core/platform/Prerequisites.h"
#include "assets/Asset.h"

namespace sfmx
{

class AssetFileReader;

class SFMX_UTILITY_EXPORT AchievementAsset : public AssetT<AchievementAsset>
{
  public:

  /**
   * @brief Decode package into achievement, stamping metadata and
   *        flipping @ref state to @ref AssetState::kLoaded / @ref
   *        AssetState::kFailed.
   * @return True on success. The codec calls this; not for the game loop.
   *
   * Synchronous path = @ref decodeCPU followed by @ref finalize, both on the caller's
   * thread (the current, GL-owning thread).
   */
  bool
  decodeFrom(AssetFileReader& reader) override;


  NODISCARD FORCEINLINE UUID getAchievementID() const { 
    return UUID::createFromName(m_name);
  }

  NODISCARD FORCEINLINE String getAchievementName() const {
    return m_name;
  }

  NODISCARD FORCEINLINE String getAchievementDescription() const {
    return m_description;
  }

  NODISCARD FORCEINLINE UUID getIconID() const {
    return m_iconID;
  }


  private:

  String m_name;
  String m_description;
  UUID m_iconID;

};

} // namespace sfmx
