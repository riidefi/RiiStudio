#pragma once

#include <array>
#include <core/common.h>
#include <vector>

#include "Indirect.hpp"

namespace libcube {
namespace gx {

enum class TevColorArg {
  cprev,
  aprev,
  c0,
  a0,
  c1,
  a1,
  c2,
  a2,
  texc,
  texa,
  rasc,
  rasa,
  one,
  half,
  konst,
  zero
};

enum class TevAlphaArg { aprev, a0, a1, a2, texa, rasa, konst, zero };

enum class TevBias {
  zero,     //!< As-is
  add_half, //!< Add middle gray
  sub_half  //!< Subtract middle gray
};

enum class TevReg {
  prev,
  reg0,
  reg1,
  reg2,

  reg3 = prev
};

enum class TevColorOp {
  add,
  subtract,

  comp_r8_gt = 8,
  comp_r8_eq,
  comp_gr16_gt,
  comp_gr16_eq,
  comp_bgr24_gt,
  comp_bgr24_eq,
  comp_rgb8_gt,
  comp_rgb8_eq
};

enum class TevScale { scale_1, scale_2, scale_4, divide_2 };

enum class TevAlphaOp {
  add,
  subtract,

  comp_r8_gt = 8,
  comp_r8_eq,
  comp_gr16_gt,
  comp_gr16_eq,
  comp_bgr24_gt,
  comp_bgr24_eq,
  // Different from ColorOp
  comp_a8_gt,
  comp_a8_eq
};

enum class ColorComponent { r = 0, g, b, a };

enum class TevKColorSel {
  const_8_8,
  const_7_8,
  const_6_8,
  const_5_8,
  const_4_8,
  const_3_8,
  const_2_8,
  const_1_8,

  k0 = 12,
  k1,
  k2,
  k3,
  k0_r,
  k1_r,
  k2_r,
  k3_r,
  k0_g,
  k1_g,
  k2_g,
  k3_g,
  k0_b,
  k1_b,
  k2_b,
  k3_b,
  k0_a,
  k1_a,
  k2_a,
  k3_a
};

enum class TevKAlphaSel {
  const_8_8,
  const_7_8,
  const_6_8,
  const_5_8,
  const_4_8,
  const_3_8,
  const_2_8,
  const_1_8,

  k0_r = 16,
  k1_r,
  k2_r,
  k3_r,
  k0_g,
  k1_g,
  k2_g,
  k3_g,
  k0_b,
  k1_b,
  k2_b,
  k3_b,
  k0_a,
  k1_a,
  k2_a,
  k3_a
};

enum class ColorSelChanLow {
  color0a0,
  color1a1,

  ind_alpha = 5,
  normalized_ind_alpha, // ind_alpha in range [0, 255]
  null                  // zero
};
enum class ColorSelChanApi {
  color0,
  color1,
  alpha0,
  alpha1,
  color0a0,
  color1a1,
  zero,

  ind_alpha,
  normalized_ind_alpha,
  null = 0xFF
};

struct TevStage {
  // RAS1_TREF
  ColorSelChanApi rasOrder = ColorSelChanApi::null;
  u8 texMap = 0;
  u8 texCoord = 0;
  u8 rasSwap = 0;
  u8 texMapSwap = 0;

  struct ColorStage {
    // KSEL
    TevKColorSel constantSelection = TevKColorSel::k0;
    // COLOR_ENV
    TevColorArg a = TevColorArg::zero;
    TevColorArg b = TevColorArg::zero;
    TevColorArg c = TevColorArg::zero;
    TevColorArg d = TevColorArg::cprev;
    TevColorOp formula = TevColorOp::add;
    TevBias bias = TevBias::zero;
    TevScale scale = TevScale::scale_1;
    bool clamp = true;
    TevReg out = TevReg::prev;

    bool operator==(const ColorStage& rhs) const noexcept {
      return constantSelection == rhs.constantSelection && a == rhs.a &&
             b == rhs.b && c == rhs.c && d == rhs.d && formula == rhs.formula &&
             bias == rhs.bias && scale == rhs.scale && clamp == rhs.clamp &&
             out == rhs.out;
    }
  } colorStage;
  struct AlphaStage {
    TevAlphaArg a = TevAlphaArg::zero;
    TevAlphaArg b = TevAlphaArg::zero;
    TevAlphaArg c = TevAlphaArg::zero;
    TevAlphaArg d = TevAlphaArg::aprev;
    TevAlphaOp formula = TevAlphaOp::add;
    // KSEL
    TevKAlphaSel constantSelection = TevKAlphaSel::k0_a;
    TevBias bias = TevBias::zero;
    TevScale scale = TevScale::scale_1;
    bool clamp = true;
    TevReg out = TevReg::prev;

    bool operator==(const AlphaStage& rhs) const noexcept {
      return constantSelection == rhs.constantSelection && a == rhs.a &&
             b == rhs.b && c == rhs.c && d == rhs.d && formula == rhs.formula &&
             bias == rhs.bias && scale == rhs.scale && clamp == rhs.clamp &&
             out == rhs.out;
    }
  } alphaStage;
  struct IndirectStage {
    u8 indStageSel{0}; // TODO: Ind tex stage sel
    IndTexFormat format{IndTexFormat::_8bit};
    IndTexBiasSel bias{IndTexBiasSel::none};
    IndTexMtxID matrix{IndTexMtxID::off};
    IndTexWrap wrapU{IndTexWrap::off};
    IndTexWrap wrapV{IndTexWrap::off};

    bool addPrev{false};
    bool utcLod{false};
    IndTexAlphaSel alpha{IndTexAlphaSel::off};

    bool operator==(const IndirectStage& stage) const noexcept {
      return indStageSel == stage.indStageSel && format == stage.format &&
             bias == stage.bias && matrix == stage.matrix &&
             wrapU == stage.wrapU && wrapV == stage.wrapV &&
             addPrev == stage.addPrev && utcLod == stage.utcLod &&
             alpha == stage.alpha;
    }
  } indirectStage;

  bool operator==(const TevStage& rhs) const noexcept {
    return rasOrder == rhs.rasOrder && texMap == rhs.texMap &&
           texCoord == rhs.texCoord && rasSwap == rhs.rasSwap &&
           texMapSwap == rhs.texMapSwap && colorStage == rhs.colorStage &&
           alphaStage == rhs.alphaStage && indirectStage == rhs.indirectStage;
  }
};

// SWAP table
struct SwapTableEntry {
  ColorComponent r, g, b, a;

  bool operator==(const SwapTableEntry& rhs) const noexcept {
    return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
  }

  ColorComponent lookup(ColorComponent other) const {
    switch (other) {
    case ColorComponent::r:
      return r;
    case ColorComponent::g:
      return g;
    case ColorComponent::b:
      return b;
    case ColorComponent::a:
      return a;
    }
  }
};

struct Shader {
  // Fixed-size DL
  std::array<SwapTableEntry, 4> mSwapTable;
  std::array<IndOrder, 4> mIndirectOrders;

  // Variable-sized DL
  std::vector<TevStage> mStages;

  bool operator==(const Shader& rhs) const noexcept {
    return mSwapTable == rhs.mSwapTable &&
           mIndirectOrders == rhs.mIndirectOrders && mStages == rhs.mStages;
  }

  Shader() {
    mSwapTable = {
        SwapTableEntry{ColorComponent::r, ColorComponent::g, ColorComponent::b,
                       ColorComponent::a},
        SwapTableEntry{ColorComponent::r, ColorComponent::r, ColorComponent::r,
                       ColorComponent::a},
        SwapTableEntry{ColorComponent::g, ColorComponent::g, ColorComponent::g,
                       ColorComponent::a},
        SwapTableEntry{ColorComponent::b, ColorComponent::b, ColorComponent::b,
                       ColorComponent::a},
    };
    mStages.push_back(TevStage{

    });
  }
};

} // namespace gx
} // namespace libcube
