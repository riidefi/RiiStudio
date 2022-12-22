#pragma once

#include <core/common.h>

namespace librii::gx {

enum class Comparison {
  NEVER,
  LESS,
  EQUAL,
  LEQUAL,
  GREATER,
  NEQUAL,
  GEQUAL,
  ALWAYS
};
enum class CullMode : u32 { None, Front, Back, All };

enum class DisplaySurface { Both, Back, Front, Neither };

inline DisplaySurface cullModeToDisplaySurfacee(CullMode c) {
  return static_cast<DisplaySurface>(4 - static_cast<int>(c));
}

enum class FogType {
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
  OrthographicInverseQuadratic,
};

enum class DiffuseFn {
  None,
  Sign,
  Clamp,
};

enum class AttnFn {
  Spec,
  Spot, // distance attenuation
  None,
};

enum class SpotFn {
  Off,
  Flat,
  Cos,
  Cos2,
  Sharp,
  Ring1,
  Ring2,
};

enum class DistAttnFn {
  Off,
  Gentle,
  Medium,
  Steep,
};

} // namespace librii::gx

#include <librii/gx/AlphaCompare.hpp>
#include <librii/gx/Blend.hpp>
#include <librii/gx/Color.hpp>
#include <librii/gx/ConstAlpha.hpp>
#include <librii/gx/Indirect.hpp>
#include <librii/gx/Lighting.hpp>
#include <librii/gx/Polygon.hpp>
#include <librii/gx/Shader.hpp>
#include <librii/gx/TexGen.hpp>
#include <librii/gx/Texture.hpp>
#include <librii/gx/Vertex.hpp>
#include <librii/gx/ZMode.hpp>

#include <rsl/ArrayVector.hpp>

#include <librii/mtx/TexMtx.hpp>

using namespace librii;

namespace librii::gx {

struct IndirectStage {
  bool operator==(const IndirectStage&) const = default;

  IndirectTextureScalePair scale;
  IndOrder order;
};

//! Models a GPU material, at a high level
//! These values directly map to low-level registers
struct LowLevelGxMaterial {
  bool operator==(const LowLevelGxMaterial&) const = default;

  CullMode cullMode = CullMode::Back;

  rsl::array_vector<ChannelData, 2> chanData{};
  // Color0, Alpha0, Color1, Alpha1
  rsl::array_vector<ChannelControl, 4> colorChanControls{};

  rsl::array_vector<TexCoordGen, 8> texGens{};

  std::array<Color, 4> tevKonstColors{};
  std::array<ColorS10, 4> tevColors{}; // last is tevprev?

  bool earlyZComparison = true;
  ZMode zMode;
  AlphaComparison alphaCompare;
  BlendMode blendMode;
  DstAlpha dstAlpha;   // G3D only (though could embed it in a J3D DL)
  bool dither = false; // Only seen in J3D normally,
                       // though we could support it for G3D.
  bool xlu = false;

  rsl::array_vector<IndirectStage, 4> indirectStages{};
  rsl::array_vector<IndirectMatrix, 3> mIndMatrices{};

  SwapTable mSwapTable;
  rsl::array_vector<TevStage, 16> mStages{};

  // Notably missing are texture matrices

  LowLevelGxMaterial() { mStages.push_back(TevStage{}); }
};

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

inline glm::mat4x4
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
