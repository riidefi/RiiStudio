#pragma once

#include <librii/crate/g3d_crate.hpp>
#include <librii/crate/j3d_crate.hpp>

#include <rsl/FsDialog.hpp>

namespace riistudio::g3d {
class Collection;
struct Material;
} // namespace riistudio::g3d
namespace riistudio::j3d {
class Collection;
struct Material;
} // namespace riistudio::j3d

namespace rs::preset_helper {

Result<void> tryExportRsPresetMat(std::string path,
                                  const riistudio::g3d::Material& mat);
Result<void> tryExportRsPresetMatJ(std::string path,
                                   const riistudio::j3d::Material& mat);
Result<void> tryExportRsPreset(const riistudio::g3d::Material& mat);
Result<void> tryExportRsPresetJ(const riistudio::j3d::Material& mat);

Result<void> ApplyRSPresetToMaterialJ3D(riistudio::j3d::Collection& c,
                                        librii::j3d::MaterialData& mat,
                                        std::span<const u8> file);
Result<void> tryImportRsPreset(riistudio::g3d::Material& mat);
Result<void> tryImportRsPresetJ(riistudio::j3d::Collection& c,
                                riistudio::j3d::Material& mat);

Result<void> tryImportRsPresetJ(riistudio::j3d::Material& mat);

static inline Result<void> tryExportRsPresetALL(auto&& mats) {
  const auto path = TRY(rsl::OpenFolder("Output folder"_j, ""));
  for (auto& mat : mats) {
    TRY(tryExportRsPresetMat((path / (mat.name + ".rspreset")).string(), mat));
  }
  return {};
}
static inline Result<void> tryExportRsPresetALLJ(auto&& mats) {
  const auto path = TRY(rsl::OpenFolder("Output folder"_j, ""));
  for (auto& mat : mats) {
    TRY(tryExportRsPresetMatJ((path / (mat.name + ".bmd_rspreset")).string(),
                              mat));
  }
  return {};
}

Result<std::vector<librii::crate::CrateAnimation>> tryImportManyRsPreset();

} // namespace rs::preset_helper
