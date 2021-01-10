#pragma once

#include <frontend/widgets/IconDatabase.hpp> // IconDatabase
#include <vendor/llvm/ADT/DenseMap.h>        // llvm::DenseMap

namespace riistudio {

class IconManager {
public:
  IconManager() = default;
  ~IconManager() = default;

  void propogateIcons(kpi::ICollection& folder);
  void propogateIcons(kpi::INode& node);
  void drawImageIcon(const lib3d::Texture* tex, u32 dim) const;

private:
  IconDatabase mIconManager;
  llvm::DenseMap<const lib3d::Texture*, IconDatabase::Key> mImageIcons;
};

} // namespace riistudio
