#include "CratePreset.hpp"
#include "AdvTexConv.hpp"
#include "ErrorState.hpp"
#include "Merge.hpp"
#include "Rename.hpp"

#include <frontend/widgets/TextEdit.hpp>

#include <rsl/Defer.hpp>
#include <rsl/FsDialog.hpp>
#include <rsl/Stb.hpp>

namespace libcube::UI {

Result<void> tryReplace(riistudio::g3d::Material& mat) {
  const auto path = TRY(rsl::OpenOneFile("Select preset"_j, "",
                                         {
                                             "MDL0Mat Files",
                                             "*.mdl0mat",
                                         }));
  const auto preset = path.parent_path();
  return ApplyCratePresetToMaterial(mat, preset);
}

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
      auto ok = tryReplace(mat);
      if (!ok) {
        std::string err = "Cannot apply preset. " + ok.error();
        mErrorState.enter(std::move(err));
        // We still return true, since this could've left us in a partially
        // mutated state
      }
      return true;
    }

    return false;
  }
};

class SaveAsTEX0 : public kpi::ActionMenu<libcube::Texture, SaveAsTEX0> {
public:
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
    memcpy(tex0.data.data(), tex.getData().data(),
           std::min<u32>(tex.getEncodedSize(true), tex0.data.size()));
    auto buf = librii::crate::WriteTEX0(tex0);
    if (buf.empty()) {
      return "librii::crate::WriteTEX0 failed";
    }

    plate::Platform::writeFile(buf, path);
    return {};
  }

public:
  bool _context(libcube::Texture& tex) {
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

std::string tryExportSRT0(const librii::g3d::SrtAnimationArchive& arc) {
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

  auto buf = librii::crate::WriteSRT0(arc.write(arc));
  if (!buf) {
    return "librii::crate::WriteSRT0 failed: " + buf.error();
  }
  if (buf->empty()) {
    return "librii::crate::WriteSRT0 failed";
  }

  plate::Platform::writeFile(*buf, path);
  return {};
}
class SaveAsSRT0 : public kpi::ActionMenu<riistudio::g3d::SRT0, SaveAsSRT0> {
  bool m_export = false;
  ErrorState m_errorState{"SRT0 Export Error"};

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
      auto err = tryExportSRT0(srt);
      if (!err.empty()) {
        m_errorState.enter(std::move(err));
      }
    }

    return false;
  }
};

std::string tryExportMdl0Mat(const librii::g3d::G3dMaterialData& mat) {
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

  auto buf_ = librii::crate::WriteMDL0Mat(mat);
  if (!buf_) {
    return "librii::crate::WriteMDL0Mat failed: " + buf_.error();
  }
  auto buf(std::move(*buf_));
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

class SaveAsMDL0MatShade
    : public kpi::ActionMenu<riistudio::g3d::Material, SaveAsMDL0MatShade> {
  bool m_export = false;
  ErrorState m_errorState{"SaveAsMDL0MatShade Export Error"};

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
      auto err = tryExportMdl0Mat(srt);
      if (!err.empty()) {
        m_errorState.enter(std::move(err));
      }
    }

    return false;
  }
};

std::string tryImportTEX0(libcube::Texture& tex) {
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
class ReplaceWithTEX0
    : public kpi::ActionMenu<libcube::Texture, ReplaceWithTEX0> {
  bool m_import = false;
  ErrorState m_errorState{"TEX0 Import Error"};

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
      auto err = tryImportTEX0(tex);
      if (!err.empty()) {
        m_errorState.enter(std::move(err));
      }
      return true;
    }

    return false;
  }
};

std::string tryImportSRT0(riistudio::g3d::SRT0& srt) {
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
class ReplaceWithSRT0
    : public kpi::ActionMenu<riistudio::g3d::SRT0, ReplaceWithSRT0> {
  bool m_import = false;
  ErrorState m_errorState{"SRT0 Import Error"};

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
      auto err = tryImportSRT0(srt);
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
  std::string err;
  for (const auto& file : *files) {
    auto tex = ReadTexture(file);
    if (!tex) {
      err += tex.error() + "\n";
      continue;
    }
    OverwriteWithG3dTex(scn.getTextures().add(), *tex);
  }

  return err;
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

  std::string err;
  for (const auto& file : *files) {
    const auto& path = file.path.string();
    if (path.ends_with(".srt0")) {
      auto rep = librii::crate::ReadSRT0(file.data);
      if (!rep) {
        err += "Failed to read .srt0 at \"" + std::string(path) + "\"\n" +
               rep.error() + "\n";
        continue;
      }
      static_cast<librii::g3d::SrtAnimationArchive&>(scn.getAnim_Srts().add()) =
          *rep;
    }
  }

  return err;
}

Result<void> tryImportRsPreset(riistudio::g3d::Material& mat) {
  const auto file = TRY(rsl::ReadOneFile("Select preset"_j, "",
                                         {
                                             "rspreset Files",
                                             "*.rspreset",
                                         }));
  return ApplyRSPresetToMaterial(mat, file.data);
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
      auto ok = tryImportRsPreset(mat);
      if (!ok) {
        m_errorState.enter(std::move(ok.error()));
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
  if (!buf) {
    return "Failed to write RSPreset";
  }
  if (buf->empty()) {
    return "librii::crate::WriteRSPreset failed";
  }

  plate::Platform::writeFile(*buf, path);
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
        rsl::ErrorDialogFmt("Failed to export preset for material {}: {}",
                            mat.name, err);
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
      auto ok = tryExportRsPresetALL(model.getMaterials());
      if (!ok) {
        m_errorState.enter(std::move(ok.error()));
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
      auto parentId = col->indexOf(bone.getName());
      child->mParent = parentId;
      bone.mChildren.push_back(childId);
      return kpi::CHANGE_NEED_RESET;
    }

    return kpi::NO_CHANGE;
  }
};

[[nodiscard]] Result<void> optimizeMesh(libcube::IndexedPolygon& mesh) {
  if (!mesh.childOf) {
    return std::unexpected("Mesh is an orphan");
  }
  auto* mdl = dynamic_cast<libcube::Model*>(mesh.childOf);
  if (!mdl) {
    return std::unexpected("Mesh is not a parent of a model");
  }

  auto imd = TRY(riistudio::rhst::decompileMesh(mesh, *mdl));
  imd = TRY(librii::rhst::MeshUtils::TriangulateMesh(imd));
  for (auto& mp : imd.matrix_primitives) {
    TRY(librii::rhst::StripifyTriangles(mp));
  }
  TRY(riistudio::rhst::compileMesh(mesh, imd, *mdl, false, false));
  mesh.nextGenerationId();

  return {};
}

struct OptimizeMesh
    : public kpi::ActionMenu<libcube::IndexedPolygon, OptimizeMesh> {
  bool m_do = false;
  bool _context(libcube::IndexedPolygon&) {
    if (ImGui::MenuItem("Optimize mesh"_j)) {
      m_do = true;
    }

    return false;
  }

  kpi::ChangeType _modal(libcube::IndexedPolygon& mesh) {
    if (m_do) {
      m_do = false;

      auto ok = optimizeMesh(mesh);
      if (!ok) {
        rsl::ErrorDialogFmt("Failed to optimize mesh: {}", ok.error());
      }
      return kpi::CHANGE_NEED_RESET;
    }

    return kpi::NO_CHANGE;
  }
};

void InstallCrate() {
  auto& action_menu = kpi::ActionMenuManager::get();
  action_menu.addMenu(std::make_unique<AddChild>());
  action_menu.addMenu(std::make_unique<riistudio::frontend::RenameNode>());
  action_menu.addMenu(std::make_unique<SaveAsTEX0>());
  if (rsl::FileDialogsSupported()) {
    action_menu.addMenu(std::make_unique<ReplaceWithTEX0>());
  }
  action_menu.addMenu(std::make_unique<SaveAsSRT0>());
  action_menu.addMenu(std::make_unique<MakeRsPreset>());
  action_menu.addMenu(std::make_unique<MakeRsPresetALL>());
  action_menu.addMenu(std::make_unique<ImportPresetsAction>());
  action_menu.addMenu(std::make_unique<SaveAsMDL0MatShade>());
  action_menu.addMenu(std::make_unique<AdvTexConvAction>());
  action_menu.addMenu(std::make_unique<OptimizeMesh>());
  if (rsl::FileDialogsSupported()) {
    action_menu.addMenu(std::make_unique<ApplyRsPreset>());
    action_menu.addMenu(std::make_unique<CrateReplaceAction>());
    action_menu.addMenu(std::make_unique<ReplaceWithSRT0>());
    action_menu.addMenu(std::make_unique<ImportTexturesAction>());
    action_menu.addMenu(std::make_unique<ImportSRTAction>());
  }
}

} // namespace libcube::UI
