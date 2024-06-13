#pragma once

namespace librii::gx {

enum class IndTexFormat {
  _8bit,
  _5bit,
  _4bit,
  _3bit,
};

enum class IndTexBiasSel { none, s, t, st, u, su, tu, stu };
struct IndTexBiasSel_H {
  bool s = false;
  bool t = false;
  bool u = false;

  IndTexBiasSel_H(IndTexBiasSel x) {
    int raw = static_cast<int>(x);
    assert(raw >= 0b000 && raw <= 0b111);
    s = raw & 0b001;
    t = raw & 0b010;
    u = raw & 0b100;
  }
  operator IndTexBiasSel() const {
    int raw = 0;
    raw += s ? 1 : 0;
    raw += t ? 2 : 0;
    raw += u ? 4 : 0;
    return static_cast<IndTexBiasSel>(raw);
  }
};
enum class IndTexAlphaSel { off, s, t, u };
enum class IndTexMtxID {
  off,
  _0,
  _1,
  _2,

  s0 = 5,
  s1,
  s2,

  t0 = 9,
  t1,
  t2
};
enum class IndTexWrap { off, _256, _128, _64, _32, _16, _0 };

struct IndirectTextureScalePair {
  enum class Selection {
    x_1,   //!< 1     (2 ^  0) (<< 0)
    x_2,   //!< 1/2   (2 ^ -1) (<< 1)
    x_4,   //!< 1/4   (2 ^ -2) (<< 2)
    x_8,   //!< 1/8   (2 ^ -3) (<< 3)
    x_16,  //!< 1/16  (2 ^ -4) (<< 4)
    x_32,  //!< 1/32  (2 ^ -5) (<< 5)
    x_64,  //!< 1/64  (2 ^ -6) (<< 6)
    x_128, //!< 1/128 (2 ^ -7) (<< 7)
    x_256  //!< 1/256 (2 ^ -8) (<< 8)
  };
  Selection U{Selection::x_1};
  Selection V{Selection::x_1};

  bool operator==(const IndirectTextureScalePair& rhs) const noexcept = default;
};
struct IndirectMatrix {
  glm::vec2 scale{0.5f, 0.5f};
  f32 rotate{0.0f};
  glm::vec2 trans{0.0f, 0.0f};

  int quant = 1; // Exponent selection

  enum Method {
    Warp,
    // G3D only:
    NormalMap,
    NormalMapSpec,
    Fur,
  };

  Method method = Method::Warp;

  // G3D Only:
  s8 refLight = -1; // For normal maps

  bool operator==(const IndirectMatrix& rhs) const noexcept = default;
  // TODO: Verify with glm decompose
  glm::mat3x2 compute() const {
    const auto theta = rotate / 180.0f * 3.141592f;
    const auto sinR = sin(theta);
    const auto cosR = cos(theta);
    const f32 center = 0.0f;

    glm::mat3x2 dst;

    dst[0][0] = scale[0] * cosR;
    dst[1][0] = scale[0] * -sinR;
    dst[2][0] = trans[0] + center + scale[0] * (sinR * center - cosR * center);

    dst[0][1] = scale[1] * sinR;
    dst[1][1] = scale[1] * cosR;
    dst[2][1] =
        trans[1] + center + -scale[1] * (-sinR * center + cosR * center);

    return dst;
  }
};
// The material part
struct IndirectSetting {
  IndirectTextureScalePair texScale;
  IndirectMatrix mtx;

  IndirectSetting() {}
  IndirectSetting(const IndirectTextureScalePair& s, const IndirectMatrix& m)
      : texScale(s), mtx(m) {}

  bool operator==(const IndirectSetting& rhs) const = default;
};

// Used in TEV settings -- one per stage
struct IndOrder {
  u8 refMap = 0;
  u8 refCoord = 0;

  bool operator==(const IndOrder& rhs) const = default;
};

static inline constexpr IndOrder NullOrder =
    IndOrder{.refMap = 0xFF, .refCoord = 0xFF};

} // namespace librii::gx
