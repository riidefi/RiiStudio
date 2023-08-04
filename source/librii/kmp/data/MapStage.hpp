#pragma once

#include <array>
#include <bit>
#include <core/common.h> // u16
#include <glm/vec3.hpp>  // glm::vec3
#include <span>

namespace librii::kmp {

enum class StartPosition {
  Standard,
  Near,
};
enum class Corner {
  Left,
  Right,
};

struct Stage {
  u8 mLapCount{3};
  Corner mCorner{Corner::Left};
  StartPosition mStartPosition{StartPosition::Standard};
  u8 mFlareTobi{true}; //!< If the flash "jumps"
  struct lensFlareOptions_t {
    bool operator==(const lensFlareOptions_t&) const = default;
    u8 a{0}, r{0xe6}, g{0xe6}, b{0xe6};
  } mLensFlareOptions;

  // --- Pre Revision 2320: End of Structure

  u8 mUnk08{0x32};
  u8 _;
  u16 mSpeedModifier; //!< Used by the speed modifier cheat code as the
                      //!< two most significant bytes of a f32. (Originally
                      //!< implicit pad)

  bool operator==(const Stage&) const = default;
};

constexpr inline f32 DecodeTruncatedBigFloat(u16 sig) {
  // Sig holds the two most significant bytes of the float in big-endian
  const u8 little_float[] = {0, 0, static_cast<u8>(sig & 0xff),
                             static_cast<u8>(sig >> 8)};

  return std::bit_cast<f32>(little_float);
}

constexpr inline u16 EncodeTruncatedBigFloat(f32 fl) {
  // Sig are the two most significant bytes of the float in big-endian
  const auto little_float = std::bit_cast<std::array<u8, 4>>(fl);

  return (little_float[3] << 8) | little_float[2];
}
static_assert(DecodeTruncatedBigFloat(EncodeTruncatedBigFloat(1.0f)) == 1.0f);

inline std::array<f32, 4>
DecodeLensFlareOpts(const librii::kmp::Stage::lensFlareOptions_t& opt) {
  return {
      opt.r / 255.0f,
      opt.g / 255.0f,
      opt.b / 255.0f,
      opt.a / 255.0f,
  };
}

inline librii::kmp::Stage::lensFlareOptions_t
EncodeLensFlareOpts(std::span<const f32, 4> clr) {
  return {.a = static_cast<u8>(clr[3] * 255.0f),
          .r = static_cast<u8>(clr[0] * 255.0f),
          .g = static_cast<u8>(clr[1] * 255.0f),
          .b = static_cast<u8>(clr[2] * 255.0f)};
}

} // namespace librii::kmp
