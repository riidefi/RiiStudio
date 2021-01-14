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
} // namespace librii::gx

#include <librii/gx/AlphaCompare.hpp>
#include <librii/gx/Blend.hpp>
#include <librii/gx/Color.hpp>
#include <librii/gx/ConstAlpha.hpp>
#include <librii/gx/Indirect.hpp>
#include <librii/gx/Lighting.hpp>
#include <librii/gx/Shader.hpp>
#include <librii/gx/TexGen.hpp>
#include <librii/gx/Texture.hpp>
#include <librii/gx/Vertex.hpp>
#include <librii/gx/ZMode.hpp>

#include <core/util/array_vector.hpp>

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

  riistudio::util::array_vector<ChannelData, 2> chanData{};
  // Color0, Alpha0, Color1, Alpha1
  riistudio::util::array_vector<ChannelControl, 4> colorChanControls{};

  riistudio::util::array_vector<TexCoordGen, 8> texGens{};

  std::array<Color, 4> tevKonstColors{};
  std::array<ColorS10, 4> tevColors{}; // last is tevprev?

  bool earlyZComparison = true;
  ZMode zMode;
  AlphaComparison alphaCompare;
  BlendMode blendMode;
  bool dither = false; // Only seen in J3D normally,
                       // though we could support it for G3D.
  bool xlu = false;

  riistudio::util::array_vector<IndirectStage, 4> indirectStages{};
  riistudio::util::array_vector<IndirectMatrix, 3> mIndMatrices{};

  SwapTable mSwapTable;
  riistudio::util::array_vector<TevStage, 16> mStages{};

  // Notably missing are texture matrices

  LowLevelGxMaterial() { mStages.push_back(TevStage{}); }
};

} // namespace librii::gx
