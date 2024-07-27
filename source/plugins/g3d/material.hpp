#pragma once

#include <core/common.h>
#include <filesystem>
#include <glm/vec3.hpp>
#include <librii/crate/g3d_crate.hpp>
#include <librii/g3d/data/MaterialData.hpp>
#include <plugins/gc/Export/Material.hpp>
#include <string>
#include <vector>

namespace kpi {
class IDocumentNode;
}

namespace riistudio::g3d {

struct Material : public librii::g3d::G3dMaterialData,
                  public libcube::IGCMaterial,
                  public virtual kpi::IObject {
  GCMaterialData& getMaterialData() override { return *this; }
  const GCMaterialData& getMaterialData() const override { return *this; }
  const libcube::Texture* getTexture(const libcube::Scene& scn,
                                     const std::string& id) const override;

  s64 getId() const override { return id; }

  bool operator==(const Material& rhs) const {
    return G3dMaterialData::operator==(rhs);
  }
  Material& operator=(const Material& rhs) {
    static_cast<libcube::GCMaterialData&>(*this) =
        static_cast<const libcube::GCMaterialData&>(rhs);
    return *this;
  }
};
[[nodiscard]] Result<void> ApplyCratePresetToMaterial(
    librii::g3d::G3dMaterialData& mat, librii::crate::CrateAnimation anim,
    bool overwrite_tex_same_name, librii::g3d::Archive* scene);
[[nodiscard]] Result<void>
ApplyCratePresetToMaterial(riistudio::g3d::Material& mat,
                           librii::crate::CrateAnimation anim,
                           bool overwrite_tex_same_name = true);
[[nodiscard]] Result<void>
ApplyCratePresetToMaterial(riistudio::g3d::Material& mat,
                           const std::filesystem::path& preset_folder);
[[nodiscard]] Result<void>
ApplyRSPresetToMaterial(riistudio::g3d::Material& mat,
                        std::span<const u8> file);

[[nodiscard]] Result<librii::crate::CrateAnimation>
CreatePresetFromMaterial(const riistudio::g3d::Material& mat,
                         std::string_view metadata = "");

} // namespace riistudio::g3d
