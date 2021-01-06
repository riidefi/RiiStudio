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

//! Models a GPU material, at a high level
//! These values directly map to low-level registers
struct LowLevelGxMaterial {
  gx::CullMode cullMode = librii::gx::CullMode::Back;

  // Gen Info counts are implied:
  // - The material shouldn't hold unused entries:
  // - nColorChan -> (this one is confusing)
  // - nTexGen -> texGens.size()
  // - nTevStage -> shader.stages.size()
  // - nIndStage -> mIndScales.size() and shader.mIndOrders.size()

  riistudio::util::array_vector<gx::ChannelData, 2> chanData;
  // Color0, Alpha0, Color1, Alpha1
  riistudio::util::array_vector<gx::ChannelControl, 4> colorChanControls;

  riistudio::util::array_vector<gx::TexCoordGen, 8> texGens;

  std::array<gx::Color, 4> tevKonstColors;
  std::array<gx::ColorS10, 4> tevColors; // last is tevprev?

  bool earlyZComparison = true;
  gx::ZMode zMode;
  gx::AlphaComparison alphaCompare;
  gx::BlendMode blendMode;
  bool dither = false;

  std::vector<gx::IndirectTextureScalePair> mIndScales;
  std::vector<gx::IndirectMatrix> mIndMatrices;

  gx::Shader shader;

  // Notably missing are texture matrices
};

} // namespace librii::gx
