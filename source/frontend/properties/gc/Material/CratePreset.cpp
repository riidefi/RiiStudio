#include <vendor/FileDialogues.hpp>

#include "Common.hpp"

#include <core/kpi/ActionMenu.hpp>
#include <core/util/oishii.hpp>
#include <librii/crate/g3d_crate.hpp>
#include <plate/Platform.hpp>
#include <plugins/g3d/collection.hpp>

namespace libcube::UI {

struct ErrorState {
  std::string mTitle;
  std::string mError;
  bool mCloseRequested = false;

  ErrorState(std::string&& title) : mTitle(title) {}
  void enter(std::string&& err) {
    mError = err;
    ImGui::OpenPopup(mTitle.c_str());
  }
  void exit() { mCloseRequested = true; }
  void modal() {
    if (ImGui::BeginPopupModal(mTitle.c_str(), nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("%s", mError.c_str());
      const bool keep_alive = !ImGui::Button("OK") || mCloseRequested;
      if (!keep_alive) {
        ImGui::CloseCurrentPopup();
        mError.clear();
        mCloseRequested = false;
      }
      ImGui::EndPopup();
    }
  }
};

class CrateReplaceAction
    : public kpi::ActionMenu<riistudio::g3d::Material, CrateReplaceAction> {
  bool replace = false;

  ErrorState mErrorState{"Preset Error"};

public:
  // Return true if the state of obj was mutated (add to history)
  bool _context(riistudio::g3d::Material& mat) {
    if (ImGui::MenuItem("Apply .mdl0mat material preset"_j)) {
      replace = true;
    }

    return false;
  }

  bool _modal(riistudio::g3d::Material& mat) {
    mErrorState.modal();

    if (replace) {
      replace = false;
      auto err = tryReplace(mat);
      if (err.size()) {
        err = "Cannot apply preset. " + err;
        mErrorState.enter(std::move(err));
        // We still return true, since this could've left us in a partially
        // mutated state
      }
      return true;
    }

    return false;
  }

private:
  std::string tryReplace(riistudio::g3d::Material& mat) {
    auto paths = pfd::open_file("Select preset"_j, "",
                                {
                                    "MDL0Mat Files",
                                    "*.mdl0mat",
                                })
                     .result();
    if (paths.empty()) {
      return "No file was selected";
    }
    const auto preset = std::filesystem::path(paths[0]).parent_path();
    return ApplyCratePresetToMaterial(mat, preset);
  }
};

class CrateTEX0Action
    : public kpi::ActionMenu<riistudio::g3d::Texture, CrateTEX0Action> {
  bool m_export = false;
  ErrorState m_errorState{"TEX0 Export Error"};

  std::string tryExport(const riistudio::g3d::Texture& tex) {
    std::string path = tex.name + ".tex0";

    // Web version does not
    if (plate::Platform::supportsFileDialogues()) {
      path = pfd::save_file("Export Path"_j, "",
                            {
                                "TEX0 texture",
                                "*.tex0",
                            })
                 .result();
      if (path.empty()) {
        return "No file was selected";
      }
    }

    auto buf = librii::crate::WriteTEX0(tex);
    if (buf.empty()) {
      return "librii::crate::WriteTEX0 failed";
    }

    plate::Platform::writeFile(buf, path);
    return {};
  }

public:
  bool _context(riistudio::g3d::Texture&) {
    if (ImGui::MenuItem("Save as .tex0"_j)) {
      m_export = true;
    }

    return false;
  }

  bool _modal(riistudio::g3d::Texture& tex) {
    m_errorState.modal();

    if (m_export) {
      m_export = false;
      auto err = tryExport(tex);
      if (!err.empty()) {
        m_errorState.enter(std::move(err));
      }
    }

    return false;
  }
};

static rsl::expected<librii::g3d::TextureData, std::string>
ImportTex0(std::string_view path_v) {
  auto path = std::string(path_v);
  auto buf = OishiiReadFile(std::string(path));
  if (!buf) {
    return "Failed to read .tex0 at \"" + path + "\"";
  }

  auto slice = buf->slice();
  std::vector<u8> bruh(slice.begin(), slice.end());

  auto replacement = librii::crate::ReadTEX0(bruh);
  if (!replacement) {
    return "Failed to read .tex0 at \"" + path + "\"\n" + replacement.error();
  }

  return replacement;
}

class CrateTEX0ActionImport
    : public kpi::ActionMenu<riistudio::g3d::Texture, CrateTEX0ActionImport> {
  bool m_import = false;
  ErrorState m_errorState{"TEX0 Import Error"};

  std::string tryImport(riistudio::g3d::Texture& tex) {
    const auto paths = pfd::open_file("Import Path"_j, "",
                                      {
                                          "TEX0 texture",
                                          "*.tex0",
                                      })
                           .result();
    if (paths.empty()) {
      return "No file was selected";
    }
    if (paths.size() != 1) {
      return "Too many files were selected";
    }
    auto replacement = ImportTex0(paths[0]);
    if (!replacement) {
      return replacement.error();
    }

    auto name = tex.name;
    static_cast<librii::g3d::TextureData&>(tex) = *replacement;
    tex.name = name;
    tex.onUpdate();

    return {};
  }

public:
  static bool IsSupported() { return plate::Platform::supportsFileDialogues(); }

  bool _context(riistudio::g3d::Texture&) {
    if (ImGui::MenuItem("Replace with .tex0"_j)) {
      m_import = true;
    }

    return false;
  }

  bool _modal(riistudio::g3d::Texture& tex) {
    m_errorState.modal();

    if (m_import) {
      m_import = false;
      auto err = tryImport(tex);
      if (!err.empty()) {
        m_errorState.enter(std::move(err));
      }
      return true;
    }

    return false;
  }
};

kpi::DecentralizedInstaller
    CrateReplaceInstaller([](kpi::ApplicationPlugins& installer) {
      kpi::ActionMenuManager::get().addMenu(
          std::make_unique<CrateReplaceAction>());
      kpi::ActionMenuManager::get().addMenu(
          std::make_unique<CrateTEX0Action>());
      if (CrateTEX0ActionImport::IsSupported()) {
        kpi::ActionMenuManager::get().addMenu(
            std::make_unique<CrateTEX0ActionImport>());
      }
    });

} // namespace libcube::UI
