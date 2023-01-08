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
#include <rsl/Defer.hpp>
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

class SaveAsTEX0 : public kpi::ActionMenu<libcube::Texture, SaveAsTEX0> {
  bool m_export = false;
  ErrorState m_errorState{"TEX0 Export Error"};

  std::string tryExport(const libcube::Texture& tex) {
    std::string path = tex.getName() + ".tex0";

    // Web version does not
    if (plate::Platform::supportsFileDialogues()) {
      auto rep = rsl::SaveOneFile("Export Path"_j, path,
                                  {
                                      "TEX0 texture (*.tex0)",
                                      "*.tex0",
                                  });
      if (!rep) {
        return rep.error();
      }
      path = rep->string();
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
class SaveAsSRT0 : public kpi::ActionMenu<riistudio::g3d::SRT0, SaveAsSRT0> {
  bool m_export = false;
  ErrorState m_errorState{"SRT0 Export Error"};

  std::string tryExport(const librii::g3d::SrtAnimationArchive& arc) {
    std::string path = arc.name + ".srt0";

    // Web version does not
    if (rsl::FileDialogsSupported()) {
      auto choice = rsl::SaveOneFile("Export Path"_j, path,
                                     {
                                         "SRT0 file (*.srt0)",
                                         "*.srt0",
                                     });
      if (!choice) {
        return choice.error();
      }
      path = choice->string();
    }

    auto buf = librii::crate::WriteSRT0(arc);
    if (!buf) {
      return "librii::crate::WriteSRT0 failed: " + buf.error();
    }
    if (buf->empty()) {
      return "librii::crate::WriteSRT0 failed";
    }

    plate::Platform::writeFile(*buf, path);
    return {};
  }

public:
  bool _context(riistudio::g3d::SRT0&) {
    if (ImGui::MenuItem("Save as .srt0"_j)) {
      m_export = true;
    }

    return false;
  }

  bool _modal(riistudio::g3d::SRT0& srt) {
    m_errorState.modal();

    if (m_export) {
      m_export = false;
      auto err = tryExport(srt);
      if (!err.empty()) {
        m_errorState.enter(std::move(err));
      }
    }

    return false;
  }
};

class SaveAsMDL0MatShade
    : public kpi::ActionMenu<riistudio::g3d::Material, SaveAsMDL0MatShade> {
  bool m_export = false;
  ErrorState m_errorState{"SaveAsMDL0MatShade Export Error"};

  std::string tryExport(const librii::g3d::G3dMaterialData& mat) {
    std::string path = mat.name + ".mdl0mat";

    // Web version does not
    if (rsl::FileDialogsSupported()) {
      auto choice = rsl::SaveOneFile("Export Path"_j, path,
                                     {
                                         "MDL0Mat file (*.mdl0mat)",
                                         "*.mdl0mat",
                                     });
      if (!choice) {
        return choice.error();
      }
      path = choice->string();
    }

    auto buf = librii::crate::WriteMDL0Mat(mat);
    if (buf.empty()) {
      return "librii::crate::WriteMDL0Mat failed";
    }
    plate::Platform::writeFile(buf, path);

    {
      auto fs_path = std::filesystem::path(path);
      auto shade =
          fs_path.replace_filename(fs_path.stem().string() + ".mdl0shade");
      auto buf = librii::crate::WriteMDL0Shade(mat);
      if (buf.empty()) {
        return "librii::crate::WriteMDL0Shade failed";
      }
      plate::Platform::writeFile(buf, shade.string());
    }

    return {};
  }

public:
  bool _context(riistudio::g3d::Material&) {
    if (ImGui::MenuItem("Save as .mdl0mat/.mdl0shade"_j)) {
      m_export = true;
    }

    return false;
  }

  bool _modal(riistudio::g3d::Material& srt) {
    m_errorState.modal();

    if (m_export) {
      m_export = false;
      auto err = tryExport(srt);
      if (!err.empty()) {
        m_errorState.enter(std::move(err));
      }
    }

    return false;
  }
};

class ReplaceWithTEX0
    : public kpi::ActionMenu<libcube::Texture, ReplaceWithTEX0> {
  bool m_import = false;
  ErrorState m_errorState{"TEX0 Import Error"};

  std::string tryImport(libcube::Texture& tex) {
    const auto file = rsl::ReadOneFile("Import Path"_j, "",
                                       {
                                           "TEX0 texture",
                                           "*.tex0",
                                       });
    if (!file) {
      return file.error();
    }
    auto replacement = librii::crate::ReadTEX0(file->data);
    if (!replacement) {
      return "Failed to read .tex0 at \"" + file->path.string() + "\"\n" +
             replacement.error();
    }

    auto name = tex.getName();
    OverwriteWithG3dTex(tex, *replacement);
    tex.setName(name);
    tex.onUpdate();

    return {};
  }

public:
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
class ReplaceWithSRT0
    : public kpi::ActionMenu<riistudio::g3d::SRT0, ReplaceWithSRT0> {
  bool m_import = false;
  ErrorState m_errorState{"SRT0 Import Error"};

  std::string tryImport(riistudio::g3d::SRT0& srt) {
    const auto file = rsl::ReadOneFile("Import Path"_j, "",
                                       {
                                           "SRT0 Animation",
                                           "*.srt0",
                                       });
    if (!file) {
      return file.error();
    }
    auto replacement = librii::crate::ReadSRT0(file->data);
    auto name = srt.getName();
    static_cast<librii::g3d::SrtAnimationArchive&>(srt) = *replacement;
    srt.setName(name);

    return {};
  }

public:
  bool _context(riistudio::g3d::SRT0&) {
    if (ImGui::MenuItem("Replace with .srt0"_j)) {
      m_import = true;
    }

    return false;
  }

  bool _modal(riistudio::g3d::SRT0& srt) {
    m_errorState.modal();

    if (m_import) {
      m_import = false;
      auto err = tryImport(srt);
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

std::string tryImportManySrt(riistudio::g3d::Collection& scn) {
  const auto files = rsl::ReadManyFile("Import Path"_j, "",
                                       {
                                           "SRT0 Animations",
                                           "*.srt0",
                                       });
  if (!files) {
    return files.error();
  }

  for (const auto& file : *files) {
    const auto& path = file.path.string();
    if (path.ends_with(".srt0")) {
      auto rep = librii::crate::ReadSRT0(file.data);
      if (!rep) {
        return "Failed to read .srt0 at \"" + std::string(path) + "\"\n" +
               rep.error();
      }
      static_cast<librii::g3d::SrtAnimationArchive&>(scn.getAnim_Srts().add()) =
          *rep;
    }
  }

  return {};
}

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

class RenameNode : public kpi::ActionMenu<kpi::IObject, RenameNode> {
  bool m_rename = false;
  std::vector<char> m_buffer;

public:
  bool _context(kpi::IObject& obj) {
    // Really silly way to determine if an object can be renamed
    auto name = obj.getName();
    obj.setName(obj.getName() + "_");
    bool can_rename = obj.getName() != name;
    obj.setName(name);

    if (can_rename && ImGui::MenuItem("Rename"_j)) {
      m_rename = true;
      m_buffer.resize(name.size() + 1024);
      std::fill(m_buffer.begin(), m_buffer.end(), '\0');
      m_buffer.insert(m_buffer.begin(), name.begin(), name.end());
    }

    return false;
  }

  kpi::ChangeType _modal(kpi::IObject& obj) {
    if (m_rename) {
      m_rename = false;
      ImGui::OpenPopup("Rename");
    }
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter());
    const auto wflags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
                        ImGuiWindowFlags_AlwaysAutoResize;
    bool open = true;
    if (ImGui::BeginPopupModal("Rename", &open, wflags)) {
      RSL_DEFER(ImGui::EndPopup());
      const auto name = obj.getName();
      auto new_name = std::string(m_buffer.data());
      bool ok = ImGui::InputText(
          name != new_name ? "Name*###Name" : "Name###Name", m_buffer.data(),
          m_buffer.size(), ImGuiInputTextFlags_EnterReturnsTrue);
      new_name = std::string(m_buffer.data());
      if (ImGui::Button("Rename") || ok) {
        if (name == new_name) {
          ImGui::CloseCurrentPopup();
          return kpi::NO_CHANGE;
        }
        obj.setName(new_name);
        ImGui::CloseCurrentPopup();
        return kpi::CHANGE;
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel") || !open) {
        ImGui::CloseCurrentPopup();
        return kpi::NO_CHANGE;
      }
    }

    return kpi::NO_CHANGE;
  }
};
std::string tryImportRsPreset(riistudio::g3d::Material& mat) {
  const auto file = rsl::ReadOneFile("Select preset"_j, "",
                                     {
                                         "rspreset Files",
                                         "*.rspreset",
                                     });
  if (!file) {
    return file.error();
  }
  return ApplyRSPresetToMaterial(mat, file->data);
}
class ApplyRsPreset
    : public kpi::ActionMenu<riistudio::g3d::Material, ApplyRsPreset> {
  bool m_import = false;
  ErrorState m_errorState{"RSPreset Import Error"};

public:
  bool _context(riistudio::g3d::Material&) {
    if (ImGui::MenuItem("Apply .rspreset material preset"_j)) {
      m_import = true;
    }

    return false;
  }

  kpi::ChangeType _modal(riistudio::g3d::Material& mat) {
    m_errorState.modal();

    if (m_import) {
      m_import = false;
      auto err = tryImportRsPreset(mat);
      if (!err.empty()) {
        m_errorState.enter(std::move(err));
      }
      return kpi::CHANGE_NEED_RESET;
    }

    return kpi::NO_CHANGE;
  }
};

std::string tryExportRsPresetMat(std::string path,
                                 const riistudio::g3d::Material& mat) {
  auto anim = CreatePresetFromMaterial(mat);
  if (!anim) {
    return "Failed to create preset bundle: " + anim.error();
  }

  auto buf = librii::crate::WriteRSPreset(*anim);
  if (buf.empty()) {
    return "librii::crate::WriteRSPreset failed";
  }

  plate::Platform::writeFile(buf, path);
  return {};
}

std::string tryExportRsPreset(const riistudio::g3d::Material& mat) {
  std::string path = mat.name + ".rspreset";
  // Web version doesn't support this
  if (rsl::FileDialogsSupported()) {
    const auto file = rsl::SaveOneFile("Select preset"_j, path,
                                       {
                                           "rspreset Files",
                                           "*.rspreset",
                                       });
    if (!file) {
      return file.error();
    }
    path = file->string();
  }
  return tryExportRsPresetMat(path, mat);
}

std::string tryExportRsPresetALL(auto&& mats) {
  if (!rsl::FileDialogsSupported()) {
    return "File dialogues unsupported on this platform";
  }
  const auto folder = pfd::select_folder("Output folder"_j, "").result();
  if (folder.empty()) {
    return folder;
  }
  auto path = std::filesystem::path(folder);
  if (!std::filesystem::exists(path)) {
    return "Folder doesn't exist";
  }
  if (!std::filesystem::is_directory(path)) {
    return "Not a folder";
  }
  for (auto& mat : mats) {
    auto ok =
        tryExportRsPresetMat((path / (mat.name + ".rspreset")).string(), mat);
    if (ok.size()) {
      return ok;
    }
  }
  return {};
}

class MakeRsPreset
    : public kpi::ActionMenu<riistudio::g3d::Material, MakeRsPreset> {
  bool m_import = false;
  ErrorState m_errorState{"RSPreset Export Error"};

public:
  bool _context(riistudio::g3d::Material&) {
    if (ImGui::MenuItem("Create material preset"_j)) {
      m_import = true;
    }

    return false;
  }

  kpi::ChangeType _modal(riistudio::g3d::Material& mat) {
    m_errorState.modal();

    if (m_import) {
      m_import = false;
      auto err = tryExportRsPreset(mat);
      if (!err.empty()) {
        m_errorState.enter(std::move(err));
      }
      return kpi::CHANGE_NEED_RESET;
    }

    return kpi::NO_CHANGE;
  }
};

class MakeRsPresetALL
    : public kpi::ActionMenu<riistudio::g3d::Model, MakeRsPresetALL> {
  bool m_import = false;
  ErrorState m_errorState{"RSPreset Export Error"};

public:
  bool _context(riistudio::g3d::Model&) {
    if (ImGui::MenuItem("Export all materials as presets")) {
      m_import = true;
    }
    return false;
  }

  kpi::ChangeType _modal(riistudio::g3d::Model& model) {
    m_errorState.modal();

    if (m_import) {
      m_import = false;
      auto err = tryExportRsPresetALL(model.getMaterials());
      if (!err.empty()) {
        m_errorState.enter(std::move(err));
      }
      return kpi::CHANGE_NEED_RESET;
    }

    return kpi::NO_CHANGE;
  }
};

struct AddChild : public kpi::ActionMenu<riistudio::g3d::Bone, AddChild> {
  bool m_do = false;
  bool _context(riistudio::g3d::Bone&) {
    if (ImGui::MenuItem("Add child"_j)) {
      m_do = true;
    }

    return false;
  }

  kpi::ChangeType _modal(riistudio::g3d::Bone& bone) {
    if (m_do) {
      m_do = false;
      auto* col = bone.collectionOf;
      col->add();
      auto childId = col->size() - 1;
      auto* child = dynamic_cast<riistudio::g3d::Bone*>(col->atObject(childId));
      assert(child);
      child->id = childId;
      child->mParent = bone.id;
      auto* mdl = dynamic_cast<riistudio::g3d::Model*>(bone.childOf);
      assert(mdl);
      bone.mChildren.push_back(childId);
      return kpi::CHANGE_NEED_RESET;
    }

    return kpi::NO_CHANGE;
  }
};

kpi::DecentralizedInstaller CrateReplaceInstaller([](kpi::ApplicationPlugins&
                                                         installer) {
  kpi::ActionMenuManager::get().addMenu(std::make_unique<AddChild>());
  kpi::ActionMenuManager::get().addMenu(std::make_unique<RenameNode>());
  kpi::ActionMenuManager::get().addMenu(std::make_unique<SaveAsTEX0>());
  kpi::ActionMenuManager::get().addMenu(std::make_unique<SaveAsSRT0>());
  kpi::ActionMenuManager::get().addMenu(std::make_unique<MakeRsPreset>());
  kpi::ActionMenuManager::get().addMenu(std::make_unique<MakeRsPresetALL>());
  kpi::ActionMenuManager::get().addMenu(std::make_unique<SaveAsMDL0MatShade>());
  if (rsl::FileDialogsSupported()) {
    kpi::ActionMenuManager::get().addMenu(std::make_unique<ApplyRsPreset>());
    kpi::ActionMenuManager::get().addMenu(
        std::make_unique<CrateReplaceAction>());
    kpi::ActionMenuManager::get().addMenu(std::make_unique<ReplaceWithTEX0>());
    kpi::ActionMenuManager::get().addMenu(std::make_unique<ReplaceWithSRT0>());
    kpi::ActionMenuManager::get().addMenu(
        std::make_unique<ImportTexturesAction>());
    kpi::ActionMenuManager::get().addMenu(std::make_unique<ImportSRTAction>());
  }
});

} // namespace libcube::UI
