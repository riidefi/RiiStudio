#include "Material.hpp"
#include <LibBadUIFramework/Plugins.hpp>
#include <librii/crate/j3d_crate.hpp>
#include <plugins/gc/Export/Scene.hpp>
#include <plugins/j3d/J3dIo.hpp>

namespace riistudio::j3d {

const libcube::Texture* Material::getTexture(const libcube::Scene& scene,
                                             const std::string& id) const {
  return scene.getTextures().findByName(id);
}

Result<void>
ApplyCratePresetToMaterialJ3D(riistudio::j3d::Collection& scene,
                              librii::j3d::MaterialData& target_mat,
                              const librii::crate::CrateAnimationJ3D& src_mat) {
  for (auto& tex : src_mat.tex) {
    if (scene.getTextures().findByName(tex.name) != nullptr) {
      rsl::warn("[Presets] Merging two textures with same name: {} (newer one "
                "pulled from {})",
                tex.name, target_mat.name);
      auto* t = scene.getTextures().findByName(tex.name);
      static_cast<librii::j3d::TextureData&>(*t) = tex;
      continue;
    }
    static_cast<librii::j3d::TextureData&>(scene.getTextures().add()) = tex;
  }
  EXPECT(scene.getModels().size() == 1);
  target_mat = src_mat.mat;
  return {};
}

Result<librii::crate::CrateAnimationJ3D>
CreatePresetFromMaterial(const riistudio::j3d::Material& mat) {
  if (!mat.childOf) {
    return std::unexpected("Material is an orphan; needs to belong to a scene");
  }
  if (!mat.childOf->childOf) {
    return std::unexpected("Model is an orphan; needs to belong to a scene");
  }
  // Model -> Scene
  auto* scene = dynamic_cast<riistudio::j3d::Collection*>(mat.childOf->childOf);
  if (!scene) {
    return std::unexpected(
        "Internal: This scene type does not support .rsmat presets. "
        "Not a BMD file?");
  }
  librii::j3d::J3dModel scn;
  readJ3dMdl(scn, scene->getModels()[0], *scene);
  return librii::crate::CreatePresetFromMaterialJ3D(mat, &scn);
}

} // namespace riistudio::j3d
