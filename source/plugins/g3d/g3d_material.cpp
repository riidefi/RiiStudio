#include "material.hpp"

#include <core/kpi/Node2.hpp>

#include <librii/crate/g3d_crate.hpp>
#include <plugins/g3d/collection.hpp>

namespace riistudio::g3d {

const libcube::Texture* Material::getTexture(const libcube::Scene& scn,
                                             const std::string& id) const {
  const auto textures = getTextureSource(scn);

  for (auto& tex : textures) {
    if (tex.getName() == id)
      return &tex;
  }

  return nullptr;
}

std::string
ApplyCratePresetToMaterial(riistudio::g3d::Material& mat,
                           const std::filesystem::path& preset_folder) {
  auto paths = librii::crate::ScanCrateAnimationFolder(preset_folder);
  if (!paths) {
    return paths.error();
  }

  auto anim = ReadCrateAnimation(*paths);
  if (!anim) {
    return anim.error();
  }

  anim->mat.name = mat.name;
  auto ret_err = RetargetCrateAnimation(*anim);
  if (ret_err.size()) {
    return ret_err;
  }

  static_cast<librii::g3d::G3dMaterialData&>(mat) = anim->mat;
  mat.onUpdate(); // Update viewport
  if (!mat.childOf) {
    return "Material is an orphan; needs to belong to a scene";
  }
  if (!mat.childOf->childOf) {
    return "Model is an orphan; needs to belong to a scene";
  }
  // Model -> Scene
  auto* scene = dynamic_cast<riistudio::g3d::Collection*>(mat.childOf->childOf);
  if (!scene) {
    return "Internal: This scene type does not support .mdl0mat presets. "
           "Not a BRRES file?";
  }
  // TODO: Texture replacement mode? Delete old textures?
  // TODO: Old animations may cause issues and aren't deleted automatically
  // (stale dependency)
  for (auto& new_tex : anim->tex) {
    if (auto* x = scene->getTextures().findByName(new_tex.name); x != nullptr) {
      std::string old_name = new_tex.name;
      new_tex.name += "_" + std::to_string(std::rand());
      TryRenameSampler(mat, old_name, new_tex.name);
    }
    auto& t = scene->getTextures().add();
    static_cast<librii::g3d::TextureData&>(t) = new_tex;
  }
  for (auto& new_srt : anim->srt) {
    if (auto* x = scene->getAnim_Srts().findByName(new_srt.name);
        x != nullptr) {
      new_srt.name += "_" + std::to_string(std::rand());
    }
    auto& t = scene->getAnim_Srts().add();
    static_cast<librii::g3d::SrtAnimationArchive&>(t) = new_srt;
  }
  return {};
}

} // namespace riistudio::g3d
