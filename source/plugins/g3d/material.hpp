#pragma once

#include <string>
#include <vector>

#include <glm/vec3.hpp>

#include <core/common.h>
#include <plugins/gc/Export/Material.hpp>

namespace kpi {
class IDocumentNode;
}

namespace riistudio::g3d {

// Note: In G3D this is per-material; in J3D it is per-texmatrix
enum class TexMatrixMode { Maya, XSI, Max };

enum class G3dMappingMode {
  Standard,
  EnvCamera,
  Projection,
  EnvLight,
  EnvSpec
};

enum class G3dIndMethod {
  Warp,
  NormalMap,
  SpecNormalMap,
  Fur,
  Res0,
  Res1,
  User0,
  User1
};

struct G3dIndConfig {
  G3dIndMethod method{G3dIndMethod::Warp};
  s8 normalMapLightRef{-1};

  bool operator==(const G3dIndConfig& rhs) const = default;
};

struct G3dMaterialData : public libcube::GCMaterialData {
  rsl::array_vector<G3dIndConfig, 4> indConfig;
  u32 flag = 0;
  u32 id; // Local
  s8 lightSetIndex = -1, fogIndex = -1;

  bool operator==(const G3dMaterialData& rhs) const = default;
};

struct Material : public G3dMaterialData,
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

} // namespace riistudio::g3d
