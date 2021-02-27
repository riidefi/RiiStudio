#pragma once

#include <librii/gx.h>

namespace librii::hx {

enum AlphaTest : int {
  ALPHA_TEST_CUSTOM,
  ALPHA_TEST_DISABLED,
  ALPHA_TEST_STENCIL,
};

constexpr gx::AlphaComparison stencil_comparison = {
    .compLeft = gx::Comparison::GEQUAL,
    .refLeft = 128,
    .op = gx::AlphaOp::_and,
    .compRight = gx::Comparison::LEQUAL,
    .refRight = 255};
constexpr gx::AlphaComparison disabled_comparison = {
    .compLeft = gx::Comparison::ALWAYS,
    .refLeft = 0,
    .op = gx::AlphaOp::_and,
    .compRight = gx::Comparison::ALWAYS,
    .refRight = 0};

inline AlphaTest classifyAlphaCompare(gx::AlphaComparison compare) {

  if (compare == librii::hx::stencil_comparison)
    return ALPHA_TEST_STENCIL;

  if (compare == librii::hx::disabled_comparison)
    return ALPHA_TEST_DISABLED;

  return ALPHA_TEST_CUSTOM;
}

constexpr gx::BlendMode no_blend = {.type = gx::BlendModeType::none,
                                    .source = gx::BlendModeFactor::src_a,
                                    .dest = gx::BlendModeFactor::inv_src_a,
                                    .logic = gx::LogicOp::_copy};
constexpr gx::BlendMode yes_blend = {.type = gx::BlendModeType::blend,
                                     .source = gx::BlendModeFactor::src_a,
                                     .dest = gx::BlendModeFactor::inv_src_a,
                                     .logic = gx::LogicOp::_copy};

constexpr gx::ZMode normal_z = {
    .compare = true, .function = gx::Comparison::LEQUAL, .update = true};
constexpr gx::ZMode blend_z = {
    .compare = true, .function = gx::Comparison::LEQUAL, .update = false};

enum PixMode : int {
  PIX_DEFAULT_OPAQUE,
  PIX_STENCIL,
  PIX_TRANSLUCENT,
  PIX_CUSTOM,
  PIX_BBX_DEFAULT
};

inline PixMode classifyPixMode(const gx::AlphaComparison& alphaCompare,
                               const gx::BlendMode& blendMode,
                               const gx::ZMode& zMode, bool earlyZComparison,
                               bool xlu) {
  const auto alpha_test = classifyAlphaCompare(alphaCompare);

  if (alpha_test == ALPHA_TEST_DISABLED && zMode == normal_z &&
      blendMode == no_blend && !xlu) {
    return earlyZComparison ? PIX_DEFAULT_OPAQUE : PIX_BBX_DEFAULT;
  }

  if (alpha_test == ALPHA_TEST_STENCIL && zMode == normal_z &&
      blendMode == no_blend && !xlu && !earlyZComparison) {
    return PIX_STENCIL;
  }

  if (alpha_test == ALPHA_TEST_DISABLED && zMode == blend_z &&
      blendMode == yes_blend && xlu && earlyZComparison) {
    return PIX_TRANSLUCENT;
  }

  return PIX_CUSTOM;
}

// Handles OPAQUE, STENCIL, TRANSLUCENT
inline bool configurePixMode(gx::LowLevelGxMaterial& o, PixMode pix_mode) {
  if (pix_mode == PIX_DEFAULT_OPAQUE) {
    o.alphaCompare = disabled_comparison;
    o.zMode = normal_z;
    o.blendMode = no_blend;
    o.xlu = false;
    o.earlyZComparison = true;
    return true;
  }

  if (pix_mode == PIX_STENCIL) {
    o.alphaCompare = stencil_comparison;
    o.zMode = normal_z;
    o.blendMode = no_blend;
    o.xlu = false;
    o.earlyZComparison = false;
    return true;
  }

  if (pix_mode == PIX_TRANSLUCENT) {
    o.alphaCompare = disabled_comparison;
    o.zMode = blend_z;
    o.blendMode = yes_blend;
    o.xlu = true;
    o.earlyZComparison = true;
    return true;
  }

  return false;
}

inline PixMode classifyPixMode(const gx::LowLevelGxMaterial& mat) {
  return classifyPixMode(mat.alphaCompare, mat.blendMode, mat.zMode,
                         mat.earlyZComparison, mat.xlu);
}

} // namespace librii::hx
