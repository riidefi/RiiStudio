#include "PresetHelper.hpp"

#include <plugins/g3d/material.hpp>
#include <plugins/j3d/Preset.hpp>

#include <rsl/FsDialog.hpp>
#include <rsl/WriteFile.hpp>

#include <librii/g3d/io/ArchiveIO.hpp>

namespace rs::preset_helper {

Result<void> tryExportRsPresetMat(std::string path,
                                  const riistudio::g3d::Material& mat) {
  auto anim = TRY(CreatePresetFromMaterial(mat));
  auto buf = TRY(librii::crate::WriteRSPreset(anim));
  TRY(rsl::WriteFile(buf, path));
  return {};
}
Result<void> tryExportRsPresetMatJ(std::string path,
                                   const riistudio::j3d::Material& mat) {
  auto anim = TRY(CreatePresetFromMaterial(mat));
  auto buf = TRY(librii::crate::WriteRSPresetJ3D(anim));
  TRY(rsl::WriteFile(buf, path));
  return {};
}

Result<void> tryExportRsPreset(const riistudio::g3d::Material& mat) {
  std::string path = mat.name + ".rspreset";
  // Web version doesn't support this
  if (rsl::FileDialogsSupported()) {
    const auto file = TRY(rsl::SaveOneFile("Select preset"_j, path,
                                           {
                                               "rspreset Files",
                                               "*.rspreset",
                                           }));
    path = file.string();
  }
  return tryExportRsPresetMat(path, mat);
}
Result<void> tryExportRsPresetJ(const riistudio::j3d::Material& mat) {
  std::string path = mat.name + ".bmd_rspreset";
  // Web version doesn't support this
  if (rsl::FileDialogsSupported()) {
    const auto file = TRY(rsl::SaveOneFile("Select preset"_j, path,
                                           {
                                               "rspreset Files",
                                               "*.bmd_rspreset",
                                           }));
    path = file.string();
  }
  return tryExportRsPresetMatJ(path, mat);
}

Result<void> tryImportRsPreset(riistudio::g3d::Material& mat) {
  const auto file = TRY(rsl::ReadOneFile("Select preset"_j, "",
                                         {
                                             "rspreset Files",
                                             "*.rspreset",
                                         }));
  return ApplyRSPresetToMaterial(mat, file.data);
}
Result<void> ApplyRSPresetToMaterialJ3D(riistudio::j3d::Collection& c,
                                        librii::j3d::MaterialData& mat,
                                        std::span<const u8> file) {
  auto anim = TRY(librii::crate::ReadRSPresetJ3D(file));
  return riistudio::j3d::ApplyCratePresetToMaterialJ3D(c, mat, anim);
}
Result<void> tryImportRsPresetJ(riistudio::j3d::Collection& c,
                                riistudio::j3d::Material& mat) {
  const auto file = TRY(rsl::ReadOneFile("Select preset"_j, "",
                                         {
                                             "bmd_rspreset Files",
                                             "*.bmd_rspreset",
                                         }));
  return ApplyRSPresetToMaterialJ3D(c, mat, file.data);
}

Result<void> tryImportRsPresetJ(riistudio::j3d::Material& mat) {
  if (!mat.childOf || !mat.childOf->childOf) {
    return std::unexpected("Invalid cast");
  }
  auto* col = dynamic_cast<riistudio::j3d::Collection*>(mat.childOf->childOf);
  if (!col) {
    return std::unexpected("Invalid cast");
  }
  return tryImportRsPresetJ(*col, mat);
}

Result<std::vector<librii::crate::CrateAnimation>> tryImportManyRsPreset() {
  const auto files =
      TRY(rsl::ReadManyFile("Import Path"_j, "",
                            {
                                ".rspreset Material/Animation presets",
                                "*.rspreset",
                            }));

  std::vector<librii::crate::CrateAnimation> presets;

  for (const auto& file : files) {
    const auto& path = file.path.string();
    if (!path.ends_with(".rspreset")) {
      continue;
    }
    auto rep = librii::crate::ReadRSPreset(file.data);
    if (!rep) {
      return std::unexpected(std::format(
          "Failed to read .rspreset at \"{}\"\n{}", path, rep.error()));
    }
    presets.push_back(*rep);
  }

  return presets;
}

} // namespace rs::preset_helper
