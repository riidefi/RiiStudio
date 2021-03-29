#pragma once

#include <array>
#include <core/common.h>
#include <glm/glm.hpp>
#include <librii/gx.h>
#include <librii/j3d/data/MaterialData.hpp>
#include <plugins/gc/Export/Material.hpp>
#include <vector>
#include <vendor/fa5/IconsFontAwesome5.h>

namespace riistudio::j3d {

class Model;

struct MaterialData : public libcube::GCMaterialData {
  using J3DSamplerData = SamplerData;

  u8 flag = 1;

  bool indEnabled = false;

  // odd data
  librii::j3d::Fog fogInfo{};
  rsl::array_vector<librii::gx::Color, 8> lightColors;
  librii::j3d::NBTScale nbtScale{};
  // unused data
  // Note: postTexGens are inferred (only enabled counts)
  rsl::array_vector<TexMatrix, 20> postTexMatrices{};
  std::array<u8, 24> stackTrash{}; //!< We have to remember this for 1:1

  bool operator==(const MaterialData& rhs) const = default;
};
struct Texture;

struct Material : public MaterialData,
                  public libcube::IGCMaterial,
                  public virtual kpi::IObject {
  // PX_TYPE_INFO_EX("J3D Material", "j3d_material", "J::Material",
  // ICON_FA_PAINT_BRUSH, ICON_FA_PAINT_BRUSH);

  ~Material() = default;
  Material() = default;

  std::string getName() const override {
    return libcube::IGCMaterial::getName();
  }

  libcube::GCMaterialData& getMaterialData() override { return *this; }
  const libcube::GCMaterialData& getMaterialData() const override {
    return *this;
  }

  const libcube::Texture* getTexture(const libcube::Scene& scene,
                                     const std::string& id) const override;
};

} // namespace riistudio::j3d
