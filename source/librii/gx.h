#pragma once

#include <core/common.h>

#include <librii/gx/AlphaCompare.hpp>
#include <librii/gx/Blend.hpp>
#include <librii/gx/Color.hpp>
#include <librii/gx/ConstAlpha.hpp>
#include <librii/gx/Indirect.hpp>
#include <librii/gx/Lighting.hpp>
#include <librii/gx/Material.hpp>
#include <librii/gx/Polygon.hpp>
#include <librii/gx/Shader.hpp>
#include <librii/gx/TexGen.hpp>
#include <librii/gx/Texture.hpp>
#include <librii/gx/Vertex.hpp>
#include <librii/gx/ZMode.hpp>
#include <librii/mtx/TexMtx.hpp>

namespace librii::gx {

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

    std::expected<glm::mat4, std::string> compute(const glm::mat4& mdl,
                                                  const glm::mat4& mvp) const;
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

static inline void TryRenameSampler(GCMaterialData& mat,
                                    const std::string& old_name,
                                    const std::string& new_name) {
  for (auto& s : mat.samplers) {
    if (s.mTexture == old_name) {
      s.mTexture = new_name;
    }
  }
}

static inline glm::mat4x4 arrayToMat4x4(const std::array<f32, 16>& m) {
  return glm::mat4x4{
      m[0], m[4], m[8],  m[12], m[1], m[5], m[9],  m[13],
      m[2], m[6], m[10], m[14], m[3], m[7], m[11], m[15],
  };
}

inline std::expected<glm::mat4, std::string>
GCMaterialData::TexMatrix::compute(const glm::mat4& mdl,
                                   const glm::mat4& mvp) const {
  auto texsrt =
      librii::mtx::computeTexSrt(scale, rotate, translate, transformModel);
  return librii::mtx::computeTexMtx(
      mdl, mvp, texsrt, arrayToMat4x4(effectMatrix), method, option);
}

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

} // namespace librii::gx
