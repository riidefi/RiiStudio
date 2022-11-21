#pragma once

#include <frontend/widgets/IconDatabase.hpp>
#include <rsl/DenseMap.hpp>

namespace riistudio {

class IconManager {
public:
  IconManager() = default;
  ~IconManager() = default;

  void propagateIcons(kpi::ICollection& folder);
  void propagateIcons(kpi::INode& node);
  // Will upload if missing
  void drawImageIcon(const lib3d::Texture* tex, u32 dim);

private:
  IconDatabase mIconManager;
  rsl::dense_map<lib3d::GenerationIDTracked::GenerationID, IconDatabase::Key>
      mImageIcons;
};

} // namespace riistudio
