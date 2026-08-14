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


  NODISCARD FORCEINLINE UUID getAchievementID(uint32 index) const { 
    return UUID::createFromName(m_ids[index].data());
  }

  NODISCARD FORCEINLINE UUID getIconID(uint32 index) const {
    return UUID::createFromName(String(m_ids[index].data()) + "_icon");
  }
  private:
  Vector<StringView> m_ids;
};

} // namespace sfmx
