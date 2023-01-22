#include "material.hpp"

#include <core/kpi/Node2.hpp>

#include <core/util/timestamp.hpp>
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
Result<void> ApplyCratePresetToMaterial(riistudio::g3d::Material& mat,
                                        librii::crate::CrateAnimation anim,
                                        bool overwrite_tex_same_name) {
  anim.mat.name = mat.name;
  auto ret_err = RetargetCrateAnimation(anim);
  if (ret_err.size()) {
    return std::unexpected(ret_err);
  }

  static_cast<librii::g3d::G3dMaterialData&>(mat) = anim.mat;
  mat.onUpdate(); // Update viewport
  if (!mat.childOf) {
    return std::unexpected("Material is an orphan; needs to belong to a scene");
  }
  if (!mat.childOf->childOf) {
    return std::unexpected("Model is an orphan; needs to belong to a scene");
  }
  // Model -> Scene
  auto* scene = dynamic_cast<riistudio::g3d::Collection*>(mat.childOf->childOf);
  if (!scene) {
    return std::unexpected(
        "Internal: This scene type does not support .mdl0mat presets. "
        "Not a BRRES file?");
  }
  // TODO: Texture replacement mode? Delete old textures?
  // TODO: Old animations may cause issues and aren't deleted automatically
  // (stale dependency)
  for (auto& new_tex : anim.tex) {
    if (!overwrite_tex_same_name) {
      if (auto* x = scene->getTextures().findByName(new_tex.name);
          x != nullptr) {
        std::string old_name = new_tex.name;
        new_tex.name += "_" + std::to_string(std::rand());
        TryRenameSampler(mat, old_name, new_tex.name);
      }
    }

    if (auto* x = scene->getTextures().findByName(new_tex.name)) {
      static_cast<librii::g3d::TextureData&>(*x) = new_tex;
      continue;
    }
    auto& t = scene->getTextures().add();
    static_cast<librii::g3d::TextureData&>(t) = new_tex;
  }
  for (auto& new_srt : anim.srt) {
    if (auto* x = scene->getAnim_Srts().findByName(new_srt.name);
        x != nullptr) {
      new_srt.name += "_" + std::to_string(std::rand());
    }
    auto& t = scene->getAnim_Srts().add();
    static_cast<librii::g3d::SrtAnimationArchive&>(t) = new_srt;
  }
  for (auto& new_clr : anim.clr) {
    if (auto* x = findByName2(scene->clrs, new_clr.name)) {
      new_clr.name += "_" + std::to_string(std::rand());
    }
    scene->clrs.emplace_back(new_clr);
  }
  for (auto& new_pat : anim.pat) {
    if (auto* x = findByName2(scene->pats, new_pat.name)) {
      new_pat.name += "_" + std::to_string(std::rand());
    }
    scene->pats.emplace_back(new_pat);
  }
  return {};
}

Result<void>
ApplyCratePresetToMaterial(riistudio::g3d::Material& mat,
                           const std::filesystem::path& preset_folder) {
  auto paths = TRY(librii::crate::ScanCrateAnimationFolder(preset_folder));
  auto anim = TRY(ReadCrateAnimation(paths));
  return ApplyCratePresetToMaterial(mat, anim);
}

Result<void> ApplyRSPresetToMaterial(riistudio::g3d::Material& mat,
                                     std::span<const u8> file) {
  auto anim = TRY(librii::crate::ReadRSPreset(file));
  return ApplyCratePresetToMaterial(mat, anim);
}

Result<librii::crate::CrateAnimation>
CreatePresetFromMaterial(const riistudio::g3d::Material& mat,
                         std::string_view metadata) {
  if (!mat.childOf) {
    return std::unexpected("Material is an orphan; needs to belong to a scene");
  }
  if (!mat.childOf->childOf) {
    return std::unexpected("Model is an orphan; needs to belong to a scene");
  }
  // Model -> Scene
  auto* scene = dynamic_cast<riistudio::g3d::Collection*>(mat.childOf->childOf);
  if (!scene) {
    return std::unexpected(
        "Internal: This scene type does not support .rsmat presets. "
        "Not a BRRES file?");
  }
  librii::g3d::Archive scn = scene->toLibRii();
  return librii::crate::CreatePresetFromMaterial(mat, &scn, metadata);
}

} // namespace riistudio::g3d
