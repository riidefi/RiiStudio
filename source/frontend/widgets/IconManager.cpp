#include "IconManager.hpp"
#include <librii/image/CheckerBoard.hpp>

namespace riistudio {

static constexpr librii::image::NullTextureData<64, 64> NullCheckerboard;

IconManager::IconManager() {
  mNullIcon =
      std::make_unique<librii::image::NullTexture<64, 64>>(NullCheckerboard);
}

void IconManager::propagateIcons(kpi::ICollection& folder) {
  for (int i = 0; i < folder.size(); ++i) {
    kpi::IObject* elem = folder.atObject(i);

    if (lib3d::Texture* tex = dynamic_cast<lib3d::Texture*>(elem);
        tex != nullptr) {
      mImageIcons.try_emplace(tex->getGenerationId(),
                              mIconManager.addIcon(*tex));
    }

    if (kpi::INode* node = dynamic_cast<kpi::INode*>(elem); node != nullptr) {
      propagateIcons(*node);
    }
  }
}
void IconManager::propagateIcons(kpi::INode& node) {
  for (int i = 0; i < node.numFolders(); ++i)
    propagateIcons(*node.folderAt(i));
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
