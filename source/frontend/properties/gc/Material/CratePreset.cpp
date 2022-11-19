#include <vendor/FileDialogues.hpp>

#include "Common.hpp"

#include <core/kpi/ActionMenu.hpp>
#include <librii/crate/g3d_crate.hpp>
#include <plugins/g3d/collection.hpp>

namespace libcube::UI {

class CrateReplaceAction
    : public kpi::ActionMenu<riistudio::g3d::Material, CrateReplaceAction> {
  bool replace = false;

public:
  // Return true if the state of obj was mutated (add to history)
  bool _context(riistudio::g3d::Material& mat) {
    if (ImGui::MenuItem("Apply .mdl0mat material preset"_j)) {
      replace = true;
    }

    return false;
  }

  bool _modal(riistudio::g3d::Material& mat) {
    if (!replace) {
      return false;
    }
    replace = false;
    static const std::vector<std::string> Mdl0MatFilter = {
        "MDL0Mat Files",
        "*.mdl0mat",
    };
    auto result = pfd::open_file("Select preset"_j, "", Mdl0MatFilter).result();
    if (result.empty()) {
      return false;
    }
    const auto path = result[0];
    auto paths = librii::crate::ScanCrateAnimationFolder(
        std::filesystem::path(path).parent_path());

    if (auto* err = std::get_if<std::string>(&paths); err != nullptr) {
      printf("ERROR: %s", err->c_str());
      return false;
    }
    auto raw_paths = *std::get_if<librii::crate::CrateAnimationPaths>(&paths);
    auto anim = ReadCrateAnimation(raw_paths);
    if (auto* err = std::get_if<std::string>(&anim); err != nullptr) {
      printf("ERROR: %s", err->c_str());
      return false;
    }
    auto raw_anim = *std::get_if<librii::crate::CrateAnimation>(&anim);
    raw_anim.mat.name = mat.name;
    auto ret_err = RetargetCrateAnimation(raw_anim);
    if (ret_err.size()) {
      printf("ERROR: %s", ret_err.c_str());
      return false;
    }
    static_cast<librii::g3d::G3dMaterialData&>(mat) = raw_anim.mat;
    mat.onUpdate(); // Update viewport
    if (!mat.childOf) {
      return false;
    }
    if (!mat.childOf->childOf) {
      return false;
    }
    // Model -> Scene
    auto* scene =
        dynamic_cast<riistudio::g3d::Collection*>(mat.childOf->childOf);
    if (!scene) {
      return false;
    }
    // TODO: Texture replacement mode? Delete old textures?
    // TODO: Old animations may cause issues and aren't deleted automatically
    // (stale dependency)
    for (auto& new_tex : raw_anim.tex) {
      if (auto* x = scene->getTextures().findByName(new_tex.name);
          x != nullptr) {
        std::string old_name = new_tex.name;
        new_tex.name += "_" + std::to_string(std::rand());
        // Update sampler reference
        for (auto& s : mat.samplers) {
          if (s.mTexture == old_name) {
            s.mTexture = new_tex.name;
          }
        }
      }
      auto& t = scene->getTextures().add();
      static_cast<librii::g3d::TextureData&>(t) = new_tex;
    }
    for (auto& new_srt : raw_anim.srt) {
      if (auto* x = scene->getAnim_Srts().findByName(new_srt.name);
          x != nullptr) {
        new_srt.name += "_" + std::to_string(std::rand());
      }
      auto& t = scene->getAnim_Srts().add();
      static_cast<librii::g3d::SrtAnimationArchive&>(t) = new_srt;
    }
    return true;
  }
};

kpi::DecentralizedInstaller
    CrateReplaceInstaller([](kpi::ApplicationPlugins& installer) {
      kpi::ActionMenuManager::get().addMenu(
          std::make_unique<CrateReplaceAction>());
    });

} // namespace libcube::UI
