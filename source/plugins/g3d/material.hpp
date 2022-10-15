#pragma once

#include <string>
#include <vector>

#include <glm/vec3.hpp>

#include <core/common.h>
#include <librii/g3d/data/MaterialData.hpp>
#include <plugins/gc/Export/Material.hpp>

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

} // namespace riistudio::g3d
