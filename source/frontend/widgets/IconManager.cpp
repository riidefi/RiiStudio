#include "IconManager.hpp"
#include <librii/image/CheckerBoard.hpp>

namespace riistudio {

static constexpr librii::image::NullTextureData<64, 64> NullCheckerboard;

IconManager* IconManager::sInstance;

IconManager::IconManager() {
  mNullIcon =
      std::make_unique<librii::image::NullTexture<64, 64>>(NullCheckerboard);
  sInstance = this;
}

void IconManager::propagateIcon(lib3d::Texture* tex) {
  mImageIcons.try_emplace(tex->getGenerationId(), mIconManager.addIcon(*tex));
}

void IconManager::drawImageIcon(const lib3d::Texture* tex, u32 dim) {
  if (tex == nullptr) {
    tex = mNullIcon.get();
  }
  if (tex == nullptr) {
    return;
  }
  if (auto icon = mImageIcons.find(tex->getGenerationId());
      icon != mImageIcons.end()) {
    mIconManager.drawIcon(icon->second, dim, dim);
  } else {
    auto key = mIconManager.addIcon(*tex);
    mImageIcons.try_emplace(tex->getGenerationId(), key);
    mIconManager.drawIcon(key, dim, dim);
  }
}

} // namespace riistudio
