#include "material.hpp"

#include <core/kpi/Node2.hpp>

namespace riistudio::g3d {

const libcube::Texture* Material::getTexture(const libcube::Scene& scn, const std::string& id) const {
  const auto textures = getTextureSource(scn);

  for (auto& tex : textures) {
    if (tex.getName() == id)
      return &tex;
  }

  return nullptr;
}

} // namespace riistudio::g3d
