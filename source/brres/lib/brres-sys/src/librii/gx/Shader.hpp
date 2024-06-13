#pragma once

#include "Indirect.hpp"
#include <algorithm>
#include <array>
#include <core/common.h>
#include <rsl/SmallVector.hpp>
#include <vector>

namespace librii::gx {

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
  comp_rgb8_eq,
};
struct TevColorOp_H {
  enum Op {
    Add,      // A + B
    Subtract, // A - B
    Mask,     // op(A[x], B[x]) ? C : 0
              // (0-(A[x] == B[x])) & C
  };
  Op op = Add;
  enum MaskOp {
    Gt,
    Eq,
  };
  MaskOp maskOp = Gt;
  enum MaskSrc {
    r8,
    gr16,
    bgr24,
    rgb8, // Special: individual channels
  };
  MaskSrc maskSrc = r8;

  TevColorOp_H(TevColorOp x) {
    int raw = static_cast<int>(x);
    assert(raw < 2 || raw >= 8);
    assert(raw >= 0 && raw <= 15);
    // operation       xyzw
    // add                0
    // subtract           1
    // comp_r8_gt      1000
    // comp_r8_eq      1001
    // comp_gr16_gt    1010
    // comp_gr16_eq    1011
    // comp_bgr24_gt   1100
    // comp_bgr24_eq   1101
    // comp_rgb8_gt    1110
    // comp_rgb8_eq    1111
    //
    // x = isCompare
    // yz = compareSrc
    // z = function
    const bool isCompare = raw & 0b1000;
    const int compareSrc = (raw >> 1) & 0b11;
    const bool function = raw & 1;

    if (isCompare) {
      op = Op::Mask;
      maskSrc = static_cast<MaskSrc>(compareSrc);
      maskOp = static_cast<MaskOp>(function);
    } else {
      op = static_cast<Op>(function);
      maskSrc = MaskSrc::r8;
      maskOp = MaskOp::Gt;
    }
    // This is implemented in hardware as GX_MAX_TEVBIAS, that is:
    // 0b00: GX_TB_ZERO
    // 0b01: GX_TB_ADDHALF
    // 0b10: GX_TB_SUBHALF
    // 0b11: * Special: Enable compare mode.
    //
    // In compare mode, the "compareSrc" field is encoded as the scale. The
    // function mode does not differ (still lowest bit).
  }
  operator TevColorOp() const {
    int raw = 0;
    raw |= (op == Op::Mask) ? 0b1000 : 0;
    raw |= (op == Op::Mask) ? static_cast<int>(maskSrc) << 1 : 0;
    raw |= (op == Op::Mask) ? static_cast<int>(maskOp) : static_cast<int>(op);
    return static_cast<TevColorOp>(raw);
  }
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

  // Not valid. For generic code
  // {
  k0 = 12,
  k1,
  k2,
  k3,
  // }

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
    using OpType = TevColorOp;

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

    bool operator==(const ColorStage& rhs) const noexcept = default;
  } colorStage;
  struct AlphaStage {
    using OpType = TevAlphaOp;

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

    bool operator==(const AlphaStage& rhs) const noexcept = default;
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

    bool operator==(const IndirectStage& stage) const noexcept = default;
  } indirectStage;

  bool operator==(const TevStage& rhs) const noexcept = default;
};

// SWAP table
struct SwapTableEntry {
  ColorComponent r = ColorComponent::r, g = ColorComponent::g,
                 b = ColorComponent::b, a = ColorComponent::a;

  bool operator==(const SwapTableEntry& rhs) const noexcept = default;

  ColorComponent lookup(ColorComponent other) const {
    switch (other) {
    case ColorComponent::r:
    default: // TODO
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

struct SwapTable : public std::array<SwapTableEntry, 4> {
  using Clr = ColorComponent;
  SwapTable() {
    (*this)[0] = {Clr::r, Clr::g, Clr::b, Clr::a};
    (*this)[1] = {Clr::r, Clr::r, Clr::r, Clr::a};
    (*this)[2] = {Clr::g, Clr::g, Clr::g, Clr::a};
    (*this)[3] = {Clr::b, Clr::b, Clr::b, Clr::a};
  }
};

} // namespace librii::gx
