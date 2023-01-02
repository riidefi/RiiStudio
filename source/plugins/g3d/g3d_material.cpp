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
std::string ApplyCratePresetToMaterial(riistudio::g3d::Material& mat,
                                       librii::crate::CrateAnimation anim) {
  anim.mat.name = mat.name;
  auto ret_err = RetargetCrateAnimation(anim);
  if (ret_err.size()) {
    return ret_err;
  }

  static_cast<librii::g3d::G3dMaterialData&>(mat) = anim.mat;
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
  for (auto& new_tex : anim.tex) {
    if (auto* x = scene->getTextures().findByName(new_tex.name); x != nullptr) {
      std::string old_name = new_tex.name;
      new_tex.name += "_" + std::to_string(std::rand());
      TryRenameSampler(mat, old_name, new_tex.name);
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
  return {};
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
  return ApplyCratePresetToMaterial(mat, *anim);
}

std::string ApplyRSPresetToMaterial(riistudio::g3d::Material& mat,
                                    std::span<const u8> file) {
  auto anim = crate::ReadRSPreset(file);
  if (!anim) {
    return anim.error();
  }
  return ApplyCratePresetToMaterial(mat, *anim);
}

rsl::expected<librii::crate::CrateAnimation, std::string>
CreatePresetFromMaterial(const riistudio::g3d::Material& mat,
                         std::string_view metadata) {
  if (!mat.childOf) {
    return std::string("Material is an orphan; needs to belong to a scene");
  }
  if (!mat.childOf->childOf) {
    return std::string("Model is an orphan; needs to belong to a scene");
  }
  // Model -> Scene
  auto* scene = dynamic_cast<riistudio::g3d::Collection*>(mat.childOf->childOf);
  if (!scene) {
    return std::string(
        "Internal: This scene type does not support .rsmat presets. "
        "Not a BRRES file?");
  }
  std::set<std::string> tex_names, srt_names;
  librii::crate::CrateAnimation result;
  result.mat = mat;
  result.metadata = metadata;
  if (metadata.empty()) {
    result.metadata =
        "RiiStudio " + std::string(RII_TIME_STAMP) + ";Source " + scene->path;
  }
  for (auto& sampler : mat.samplers) {
    auto& tex = sampler.mTexture;
    if (tex_names.contains(tex)) {
      continue;
    }
    tex_names.emplace(tex);
    auto* data = scene->getTextures().findByName(tex);
    if (data == nullptr) {
      return std::string("Failed to find referenced textures ") + tex;
    }
    result.tex.push_back(*data);
  }
  for (auto& srt : scene->getAnim_Srts()) {
    librii::g3d::SrtAnimationArchive mut = srt;
    auto it =
        std::remove_if(mut.materials.begin(), mut.materials.end(),
                       [&](auto& m) { return m.name != mat.name; });
    if (it != mut.materials.end()) {
      mut.materials.erase(it, mut.materials.end());
    }
    if (!mut.materials.empty()) {
      result.srt.push_back(mut);
    }
  }
  return result;
}

} // namespace riistudio::g3d
