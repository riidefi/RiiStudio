#include "IconManager.hpp"

namespace riistudio {

void IconManager::propogateIcons(kpi::ICollection& folder) {
  for (int i = 0; i < folder.size(); ++i) {
    kpi::IObject* elem = folder.atObject(i);

    if (lib3d::Texture* tex = dynamic_cast<lib3d::Texture*>(elem);
        tex != nullptr) {
      mImageIcons.try_emplace(tex, mIconManager.addIcon(*tex));
    }

    if (kpi::INode* node = dynamic_cast<kpi::INode*>(elem); node != nullptr) {
      propogateIcons(*node);
    }
  }
}
void IconManager::propogateIcons(kpi::INode& node) {
  for (int i = 0; i < node.numFolders(); ++i)
    propogateIcons(*node.folderAt(i));
}

void IconManager::drawImageIcon(const lib3d::Texture* tex, u32 dim) const {
  if (auto icon = mImageIcons.find(tex);
      icon != mImageIcons.end())
    mIconManager.drawIcon(icon->second, dim, dim);
}

} // namespace riistudio
