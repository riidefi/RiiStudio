#include <vendor/FileDialogues.hpp>

#include "Common.hpp"

#include <core/kpi/ActionMenu.hpp>
#include <core/util/oishii.hpp>
#include <core/util/timestamp.hpp>
#include <librii/crate/g3d_crate.hpp>
#include <librii/image/TextureExport.hpp>
#include <plate/Platform.hpp>
#include <plugins/ass/ImportTexture.hpp>
#include <plugins/g3d/collection.hpp>
#include <rsl/FsDialog.hpp>
#include <vendor/stb_image.h>

namespace libcube::UI {

void OverwriteWithG3dTex(libcube::Texture& tex,
                         const librii::g3d::TextureData& rep) {
  // Using more general approach to support J3D too
  // static_cast<librii::g3d::TextureData&>(scn.getTextures().add()) =
  //     *replacement;

  tex.setName(rep.name);
  tex.setTextureFormat(rep.format);
  tex.setWidth(rep.width);
  tex.setHeight(rep.height);
  tex.setImageCount(rep.number_of_images);
  tex.setLod(rep.custom_lod, rep.minLod, rep.maxLod);
  tex.setSourcePath(rep.sourcePath);
  tex.resizeData();
  memcpy(tex.getData(), rep.data.data(),
         std::min<u32>(tex.getEncodedSize(true), rep.data.size()));
}

static rsl::expected<librii::g3d::TextureData, std::string>
ImportTex0(std::string_view path) {
  auto buf = ReadFile(path);
  if (!buf) {
    return buf.error();
  }
  auto replacement = librii::crate::ReadTEX0(*buf);
  if (!replacement) {
    return "Failed to read .tex0 at \"" + std::string(path) + "\"\n" +
           replacement.error();
  }

  return replacement;
}

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
    const auto path = rsl::OpenOneFile("Select preset"_j, "",
                                       {
                                           "MDL0Mat Files",
                                           "*.mdl0mat",
                                       });
    if (!path) {
      return path.error();
    }
    const auto preset = path->parent_path();
    return ApplyCratePresetToMaterial(mat, preset);
  }
};

class CrateTEX0Action
    : public kpi::ActionMenu<libcube::Texture, CrateTEX0Action> {
  bool m_export = false;
  ErrorState m_errorState{"TEX0 Export Error"};

  std::string tryExport(const libcube::Texture& tex) {
    std::string path = tex.getName() + ".tex0";

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

    librii::g3d::TextureData tex0{
        .name = tex.getName(),
        .format = tex.getTextureFormat(),
        .width = tex.getWidth(),
        .height = tex.getHeight(),
        .number_of_images = tex.getImageCount(),
        .custom_lod = true,
        .minLod = tex.getMinLod(),
        .maxLod = tex.getMaxLod(),
        .sourcePath = tex.getSourcePath(),
        .data = {},
    };
    tex0.data.resize(tex.getEncodedSize(true));
    memcpy(tex0.data.data(), tex.getData(),
           std::min<u32>(tex.getEncodedSize(true), tex0.data.size()));
    auto buf = librii::crate::WriteTEX0(tex0);
    if (buf.empty()) {
      return "librii::crate::WriteTEX0 failed";
    }

    plate::Platform::writeFile(buf, path);
    return {};
  }

public:
  bool _context(libcube::Texture&) {
    if (ImGui::MenuItem("Save as .tex0"_j)) {
      m_export = true;
    }

    return false;
  }

  bool _modal(libcube::Texture& tex) {
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

class CrateTEX0ActionImport
    : public kpi::ActionMenu<libcube::Texture, CrateTEX0ActionImport> {
  bool m_import = false;
  ErrorState m_errorState{"TEX0 Import Error"};

  std::string tryImport(libcube::Texture& tex) {
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

    auto name = tex.getName();
    OverwriteWithG3dTex(tex, *replacement);
    tex.setName(name);
    tex.onUpdate();

    return {};
  }

public:
  static bool IsSupported() { return plate::Platform::supportsFileDialogues(); }

  bool _context(libcube::Texture&) {
    if (ImGui::MenuItem("Replace with .tex0"_j)) {
      m_import = true;
    }

    return false;
  }

  bool _modal(libcube::Texture& tex) {
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

std::string tryImportMany(libcube::Scene& scn) {
  const auto files = rsl::ReadManyFile("Import Path"_j, "",
                                       {
                                           "Image files",
                                           "*.tex0;*.png;*.tga;*.jpg;*.bmp",
                                           "TEX0 Files",
                                           "*.tex0",
                                           "PNG Files",
                                           "*.png",
                                           "TGA Files",
                                           "*.tga",
                                           "JPG Files",
                                           "*.jpg",
                                           "BMP Files",
                                           "*.bmp",
                                           "All Files",
                                           "*",
                                       });
  if (!files) {
    return files.error();
  }

  for (const auto& file : *files) {
    const auto& path = file.path.string();
    if (path.ends_with(".tex0")) {
      auto rep = librii::crate::ReadTEX0(file.data);
      if (!rep) {
        return "Failed to read .tex0 at \"" + std::string(path) + "\"\n" +
               rep.error();
      }
      OverwriteWithG3dTex(scn.getTextures().add(), *rep);

      continue;
    }

    int width, height, channels;
    unsigned char* image =
        stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!image) {
      return "STB failed to parse image. Unsupported file format?";
    }
    if (width > 1024) {
      return "Width " + std::to_string(width) + " exceeds maximum of 1024";
    }
    if (height > 1024) {
      return "Height " + std::to_string(height) + " exceeds maximum of 1024";
    }
    if (!is_power_of_2(width)) {
      return "Width " + std::to_string(width) + " is not a power of 2.";
    }
    if (!is_power_of_2(height)) {
      return "Height " + std::to_string(height) + " is not a power of 2.";
    }
    std::vector<u8> scratch;
    auto& tex = scn.getTextures().add();
    const bool ok = riistudio::ass::importTexture(tex, image, scratch, true, 64,
                                                  4, width, height, channels);
    if (!ok) {
      return "Failed to import texture";
    }
    tex.setName(file.path.stem().string());
  }

  return {};
}

class ImportTexturesAction
    : public kpi::ActionMenu<libcube::Scene, ImportTexturesAction> {
  bool m_import = false;
  ErrorState m_errorState{"Textures Import Error"};

public:
  static bool IsSupported() { return plate::Platform::supportsFileDialogues(); }

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
      if (ImportTexturesAction::IsSupported()) {
        kpi::ActionMenuManager::get().addMenu(
            std::make_unique<ImportTexturesAction>());
      }
    });

} // namespace libcube::UI
