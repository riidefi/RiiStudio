#include "material.hpp"

#include <core/kpi/Node.hpp>

namespace riistudio::g3d {

const libcube::Texture* Material::getTexture(const std::string& id) const {
  const auto textures = getTextureSource();

  for (auto& tex : textures) {
    if (tex.getName() == id)
      return &tex;
  }

  return nullptr;
}

} // namespace riistudio::g3d
