#pragma once

#include "Common.hpp"
#include "ErrorState.hpp"

#include <LibBadUIFramework/ActionMenu.hpp>
#include <core/util/oishii.hpp>
#include <core/util/timestamp.hpp>

#include <frontend/widgets/IconManager.hpp>
#include <frontend/widgets/Lib3dImage.hpp>

#include <librii/crate/g3d_crate.hpp>
#include <librii/image/ImagePlatform.hpp>
#include <librii/image/TextureExport.hpp>
#include <librii/rhst/MeshUtils.hpp>

#include <plate/Platform.hpp>

#include <plugins/g3d/collection.hpp>
#include <plugins/rhst/RHSTImporter.hpp>

namespace libcube::UI {

std::string tryImportMany(libcube::Scene& scn);
std::string tryImportManySrt(riistudio::g3d::Collection& scn);

class ImportTexturesAction
    : public kpi::ActionMenu<libcube::Scene, ImportTexturesAction> {
  bool m_import = false;
  ErrorState m_errorState{"Textures Import Error"};

public:
  bool _context(libcube::Scene&) {
    if (ImGui::MenuItem("Import textures"_j)) {
      m_import = true;
    }

    return false;
  }

  kpi::ChangeType _modal(libcube::Scene& scn) {
    m_errorState.modal();

    if (m_import) {
      m_import = false;
      auto err = tryImportMany(scn);
      if (!err.empty()) {
        m_errorState.enter(std::move(err));
      }
      return kpi::CHANGE_NEED_RESET;
    }

    return kpi::NO_CHANGE;
  }
};

[[nodiscard]] Result<void> optimizeMesh(libcube::IndexedPolygon& mesh);
std::string tryExportMdl0Mat(const librii::g3d::G3dMaterialData& mat);

class ImportSRTAction
    : public kpi::ActionMenu<riistudio::g3d::Collection, ImportSRTAction> {
  bool m_import = false;
  ErrorState m_errorState{"SRT Import Error"};

public:
  bool _context(riistudio::g3d::Collection&) {
    if (ImGui::MenuItem("Import Texture SRT Animations"_j)) {
      m_import = true;
    }

    return false;
  }

  kpi::ChangeType _modal(riistudio::g3d::Collection& scn) {
    m_errorState.modal();

    if (m_import) {
      m_import = false;
      auto err = tryImportManySrt(scn);
      if (!err.empty()) {
        m_errorState.enter(std::move(err));
      }
      return kpi::CHANGE_NEED_RESET;
    }

    return kpi::NO_CHANGE;
  }
};

std::string tryImportTEX0(libcube::Texture& tex);
std::string tryImportSRT0(riistudio::g3d::SRT0& srt);
Result<void> tryExportSRT0(const librii::g3d::SrtAnimationArchive& arc);
Result<void> tryReplace(riistudio::g3d::Material& mat);
Result<void> tryImportRsPreset(riistudio::g3d::Material& mat);
std::string tryExportRsPreset(const riistudio::g3d::Material& mat);
std::string tryExportRsPresetMat(std::string path,
                                 const riistudio::g3d::Material& mat);
inline Result<void> tryExportRsPresetALL(auto&& mats) {
  const auto path = TRY(rsl::OpenFolder("Output folder"_j, ""));
  for (auto& mat : mats) {
    auto ok =
        tryExportRsPresetMat((path / (mat.name + ".rspreset")).string(), mat);
    if (ok.size()) {
      return std::unexpected(ok);
    }
  }
  return {};
}

} // namespace libcube::UI
