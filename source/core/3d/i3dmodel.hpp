#pragma once

#include "Bone.hpp"
#include "Material.hpp"
#include "Polygon.hpp"
#include "Texture.hpp"

namespace riistudio::lib3d {

// TODO: This should all be runtime
struct Scene {
  virtual ~Scene() = default;
};
struct Model {
  virtual ~Model() = default;
};

} // namespace riistudio::lib3d
