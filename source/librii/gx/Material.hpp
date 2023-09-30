#pragma once

#include <librii/gx/Comparison.hpp>
#include <librii/gx/Shader.hpp>
#include <librii/gx/TexGen.hpp>
#include <librii/gx/ZMode.hpp>
#include <rsl/ArrayVector.hpp>

namespace librii::gx {

enum class CullMode : u32 {
  None,
  Front,
  Back,
  All,
};

enum class DisplaySurface {
  Both,
  Back,
  Front,
  Neither,
};

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

} // namespace librii::gx
