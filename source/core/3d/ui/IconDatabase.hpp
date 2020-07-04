#pragma once

#include <core/3d/Texture.hpp> // for lib3d::Texture
#include <core/common.h>       // for u64
#include <vector>              // for vector

namespace riistudio {

class IconDatabase {
public:
  using Key = u64;

  IconDatabase(u32 iconDimension = 64);
  ~IconDatabase();

  Key addIcon(lib3d::Texture& texture);
  void drawIcon(Key id, int wd, int ht) const;

private:
  // To be a valid Icon, it must be GPU uploaded.
  struct Icon {
    u32 glId;

    Icon(Icon&& rhs) : glId(rhs.glId) { rhs.glId = -1; }
    Icon(lib3d::Texture& texture, u32 dimension);
    ~Icon();
  };

  u32 mIconDim;
  std::vector<Icon> mIcons;
};

} // namespace riistudio
