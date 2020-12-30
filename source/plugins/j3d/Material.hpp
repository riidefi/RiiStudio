#pragma once

#include <array>
#include <core/common.h>
#include <vector>

#include <glm/glm.hpp>
#include <lib_rii/gx.h>

#include <plugins/gc/Export/Material.hpp>

#include <vendor/fa5/IconsFontAwesome5.h>

namespace riistudio::j3d {

class Model;

template <typename T> using ID = int;

// FIXME: This will exist in our scene and be referenced.
// For now, for 1:1, just placed here..
struct Fog {
  enum class Type {
    None,

    PerspectiveLinear,
    PerspectiveExponential,
    PerspectiveQuadratic,
    PerspectiveInverseExponential,
    PerspectiveInverseQuadratic,

    OrthographicLinear,
    OrthographicExponential,
    OrthographicQuadratic,
    OrthographicInverseExponential,
    OrthographicInverseQuadratic
  };
  Type type = Type::None;
  bool enabled = false;
  u16 center;
  f32 startZ, endZ = 0.0f;
  f32 nearZ, farZ = 0.0f;
  librii::gx::Color color = librii::gx::Color(0xffffffff);
  std::array<u16, 10> rangeAdjTable;

  bool operator==(const Fog& rhs) const noexcept {
    return type == rhs.type && enabled == rhs.enabled && center == rhs.center &&
           startZ == rhs.startZ && endZ == rhs.endZ && nearZ == rhs.nearZ &&
           farZ == rhs.farZ && color == rhs.color &&
           rangeAdjTable == rhs.rangeAdjTable;
  }
};

struct NBTScale {
  bool enable = false;
  glm::vec3 scale = {0.0f, 0.0f, 0.0f};

  bool operator==(const NBTScale& rhs) const noexcept {
    return enable == rhs.enable && scale == rhs.scale;
  }
};

struct MaterialData : public libcube::GCMaterialData {
  struct J3DSamplerData : SamplerData {
    // Only for linking
    u16 btiId;

    bool operator==(const J3DSamplerData& rhs) const {
      if (!SamplerData::operator==(rhs))
        return false;
      if (btiId == rhs.btiId)
        return true;
      // printf("TexData does not match\n");
      return false;
    }
    std::unique_ptr<SamplerData> clone() const override {
      return std::make_unique<J3DSamplerData>(*this);
    }

    J3DSamplerData(u32 id) : btiId(id) {}
    J3DSamplerData() : btiId(-1) {}
  };

  u8 flag = 1;

  bool indEnabled = false;

  // odd data
  Fog fogInfo{};
  libcube::array_vector<librii::gx::Color, 8> lightColors;
  NBTScale nbtScale{};
  // unused data
  // Note: postTexGens are inferred (only enabled counts)
  libcube::array_vector<TexMatrix, 20> postTexMatrices{};
  std::array<u8, 24> stackTrash{}; //!< We have to remember this for 1:1

  // Note: For comparison we don't check the ID or name (or stack trash)
  bool operator==(const MaterialData& rhs) const noexcept {
    return flag == rhs.flag && indEnabled == rhs.indEnabled &&
           info == rhs.info && cullMode == rhs.cullMode &&
           earlyZComparison == rhs.earlyZComparison && zMode == rhs.zMode &&
           colorChanControls == rhs.colorChanControls &&
           chanData == rhs.chanData && lightColors == rhs.lightColors &&
           texGens == rhs.texGens && texMatrices == rhs.texMatrices &&
           samplers == rhs.samplers && postTexMatrices == rhs.postTexMatrices &&
           tevKonstColors == rhs.tevKonstColors && tevColors == rhs.tevColors &&
           shader == rhs.shader && mIndScales == rhs.mIndScales &&
           mIndMatrices == rhs.mIndMatrices && fogInfo == rhs.fogInfo &&
           alphaCompare == rhs.alphaCompare && blendMode == rhs.blendMode &&
           dither == rhs.dither && nbtScale == rhs.nbtScale;
  }
};
struct Texture;

struct Material : public MaterialData, public libcube::IGCMaterial {
  // PX_TYPE_INFO_EX("J3D Material", "j3d_material", "J::Material",
  // ICON_FA_PAINT_BRUSH, ICON_FA_PAINT_BRUSH);

  ~Material() = default;
  Material() = default;

  libcube::GCMaterialData& getMaterialData() override { return *this; }
  const libcube::GCMaterialData& getMaterialData() const override {
    return *this;
  }

  bool isXluPass() const override { return flag & 4; }
  void setXluPass(bool b) override { flag = (flag & ~4) | (b ? 4 : 0); }
  const libcube::Texture* getTexture(const std::string& id) const override;
};

} // namespace riistudio::j3d
