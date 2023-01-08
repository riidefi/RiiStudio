#include "material.hpp"

#include <core/kpi/Node2.hpp>

#include <core/util/timestamp.hpp>
#include <librii/crate/g3d_crate.hpp>
#include <plugins/g3d/collection.hpp>

namespace riistudio::g3d {

auto findByName = [](auto&& c, auto&& name) {
  auto it = std::ranges::find_if(c, [&](auto& x) { return x.name == name; });
  if (it == std::ranges::end(c)) {
    return (decltype(&*it))nullptr;
  }
  return &*it;
};

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
  for (auto& new_clr : anim.clr) {
    if (auto* x = findByName(scene->clrs, new_clr.name)) {
      new_clr.name += "_" + std::to_string(std::rand());
    }
    scene->clrs.emplace_back(new_clr);
  }
  for (auto& new_pat : anim.pat) {
    if (auto* x = findByName(scene->pats, new_pat.name)) {
      new_pat.name += "_" + std::to_string(std::rand());
    }
    scene->pats.emplace_back(new_pat);
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

std::expected<librii::crate::CrateAnimation, std::string>
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
      return std::unexpected(
          std::string("Failed to find referenced textures ") + tex);
    }
    result.tex.push_back(*data);
  }
  for (auto& srt : scene->getAnim_Srts()) {
    librii::g3d::SrtAnimationArchive mut = srt;
    std::erase_if(mut.materials, [&](auto& m) { return m.name != mat.name; });
    if (!mut.materials.empty()) {
      result.srt.push_back(mut);
    }
  }
  for (auto& clr : scene->clrs) {
    librii::g3d::BinaryClr mut = clr;
    std::erase_if(mut.materials, [&](auto& m) { return m.name != mat.name; });
    if (!mut.materials.empty()) {
      result.clr.push_back(mut);
    }
  }
  for (auto& pat : scene->pats) {
    librii::g3d::BinaryTexPat mut = pat;
    std::erase_if(mut.materials, [&](auto& m) { return m.name != mat.name; });

    std::vector<u32> referenced;
    for (auto& mat : mut.materials) {
      for (auto& s : mat.samplers) {
        if (auto* u = std::get_if<u32>(&s)) {
          EXPECT(*u < mut.tracks.size());
          auto& track = mut.tracks[*u];
          for (auto& [idx, f] : track.keyframes) {
            EXPECT(f.palette == 0, "Palettes are not supported");
            EXPECT(f.texture < mut.textureNames.size());
            referenced.emplace_back(f.texture);
          }
        } else if (auto* f = std::get_if<librii::g3d::PAT0KeyFrame>(&s)) {
          EXPECT(f->palette == 0, "Palettes are not supported");
          referenced.emplace_back(f->texture);
        } else {
          return std::unexpected("Invalid PAT0");
        }
      }
    }

    for (auto u : referenced) {
      EXPECT(u < mut.textureNames.size());
      auto& tex = mut.textureNames[u];
      if (findByName(result.tex, tex)) {
        continue;
      }
      auto* data = scene->getTextures().findByName(tex);
      if (data == nullptr) {
        return std::unexpected(
            std::format("Failed to find referenced texture {}", tex));
      }
      result.tex.push_back(*data);
    }

    if (!mut.materials.empty()) {
      result.pat.push_back(mut);
    }
  }
  return result;
}

} // namespace riistudio::g3d
