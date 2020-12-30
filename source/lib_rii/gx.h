#pragma once

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

#include <lib_rii/gx/AlphaCompare.hpp>
#include <lib_rii/gx/Blend.hpp>
#include <lib_rii/gx/Color.hpp>
#include <lib_rii/gx/ConstAlpha.hpp>
#include <lib_rii/gx/Indirect.hpp>
#include <lib_rii/gx/Lighting.hpp>
#include <lib_rii/gx/Shader.hpp>
#include <lib_rii/gx/TexGen.hpp>
#include <lib_rii/gx/Texture.hpp>
#include <lib_rii/gx/Vertex.hpp>
#include <lib_rii/gx/ZMode.hpp>

using namespace librii;
