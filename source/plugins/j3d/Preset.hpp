#pragma once

#include "J3dIo.hpp"
#include <librii/crate/j3d_crate.hpp>

namespace riistudio::j3d {

Result<librii::crate::CrateAnimationJ3D>
CreatePresetFromMaterial(const riistudio::j3d::Material& mat);

Result<void>
ApplyCratePresetToMaterialJ3D(riistudio::j3d::Collection& scene,
                              librii::j3d::MaterialData& target_mat,
                              const librii::crate::CrateAnimationJ3D& src_mat);

} // namespace riistudio::j3d
