#pragma once

#include "Texture.hpp"
#include <core/3d/i3dmodel.hpp>
#include <core/common.h>
#include <core/kpi/Node2.hpp>
#include <librii/gx.h>
#include <librii/mtx/TexMtx.hpp>
#include <rsl/ArrayVector.hpp>

namespace libcube {

class Scene;
class Model;

struct GCMaterialData : public gx::LowLevelGxMaterial {
  bool operator==(const GCMaterialData& rhs) const = default;

  std::string name;

  using CommonTransformModel = librii::mtx::CommonTransformModel;
  using CommonMappingOption = librii::mtx::CommonMappingOption;
  using CommonMappingMethod = librii::mtx::CommonMappingMethod;

  struct TexMatrix {
    // TODO: Deprecate?
    gx::TexGenType projection =
        gx::TexGenType::Matrix2x4; // Only 3x4 and 2x4 valid

    glm::vec2 scale{1.0f, 1.0f};
    f32 rotate{0.0f};
    glm::vec2 translate{0.0f, 0.0f};

    std::array<f32, 16> effectMatrix{};

    CommonTransformModel transformModel = CommonTransformModel::Maya;
    CommonMappingMethod method = CommonMappingMethod::Standard;
    CommonMappingOption option = CommonMappingOption::NoSelection;

    // G3d
    s8 camIdx = -1;
    s8 lightIdx = -1;

    bool isIdentity() const {
      // TODO -- proper float equality
      return scale == glm::vec2{1.0f, 1.0f} && rotate == 0.0f &&
             translate == glm::vec2{0.0f, 0.0f} &&
             method == CommonMappingMethod::Standard;
    }

    glm::mat4x4 compute(const glm::mat4& mdl, const glm::mat4& mvp) const;
    // TODO: Support / restriction

    bool operator==(const TexMatrix& rhs) const = default;
  };

  rsl::array_vector<TexMatrix, 10> texMatrices;

  struct SamplerData final {
    std::string mTexture;
    std::string mPalette;

    gx::TextureWrapMode mWrapU = gx::TextureWrapMode::Repeat;
    gx::TextureWrapMode mWrapV = gx::TextureWrapMode::Repeat;

    // bool bMipMap = false;
    bool bEdgeLod = false;
    bool bBiasClamp = false;

    gx::AnisotropyLevel mMaxAniso = gx::AnisotropyLevel::x1;

    gx::TextureFilter mMinFilter = gx::TextureFilter::Linear;
    gx::TextureFilter mMagFilter = gx::TextureFilter::Linear;
    f32 mLodBias = 0.0f;

    u16 btiId = 0; // Hack for J3D IO

    bool operator==(const SamplerData& rhs) const = default;
  };

  rsl::array_vector<SamplerData, 8> samplers;
};

inline void ConfigureSingleTex(GCMaterialData& mat, std::string tex_str) {
  GCMaterialData::SamplerData sampler;
  sampler.mTexture = tex_str;
  // TODO: EdgeLod/BiasClamp
  // TODO: MaxAniso, filter, bias
  mat.samplers.push_back(std::move(sampler));
  mat.samplers.resize(1);

  mat.texGens.push_back(gx::TexCoordGen{
      gx::TexGenType::Matrix3x4, gx::TexGenSrc::UV0, gx::TexMatrix::TexMatrix0,
      false, gx::PostTexMatrix::Identity});
  mat.texGens.resize(1);
  GCMaterialData::TexMatrix mtx;
  mtx.projection = gx::TexGenType::Matrix3x4;
  mtx.scale = {1, 1};
  mtx.rotate = 0;
  mtx.translate = {0, 0};
  mtx.effectMatrix = {0};
  mtx.transformModel = GCMaterialData::CommonTransformModel::Maya;
  mtx.method = GCMaterialData::CommonMappingMethod::Standard;
  mtx.option = GCMaterialData::CommonMappingOption::NoSelection;

  mat.texMatrices.push_back(std::move(mtx));
  mat.texMatrices.resize(1);

  mat.mStages[0].texMap = 0;
  mat.mStages[0].texCoord = 0;
  mat.mStages[0].colorStage.d = gx::TevColorArg::texc;
  mat.mStages[0].alphaStage.d = gx::TevAlphaArg::texa;
}

struct IGCMaterial : public riistudio::lib3d::Material {
  virtual GCMaterialData& getMaterialData() = 0;
  virtual const GCMaterialData& getMaterialData() const = 0;

  bool isXluPass() const override final { return getMaterialData().xlu; }
  void setXluPass(bool b) override final { getMaterialData().xlu = b; }

  virtual const libcube::Model* getParent() const { return nullptr; }
  std::pair<std::string, std::string> generateShaders() const override;

  virtual kpi::ConstCollectionRange<Texture>
  getTextureSource(const libcube::Scene& scn) const;
  virtual const Texture* getTexture(const libcube::Scene& scn,
                                    const std::string& id) const = 0;

  std::string getName() const override { return getMaterialData().name; }
  void setName(const std::string& name) override {
    getMaterialData().name = name;
  }

  void setMegaState(librii::gfx::MegaState& state) const override;

  void configure(librii::gfx::PixelOcclusion occlusion,
                 std::vector<std::string>& textures) override {
    // TODO
    if (textures.empty())
      return;

    ConfigureSingleTex(getMaterialData(), textures[0]);
  }
};

} // namespace libcube
