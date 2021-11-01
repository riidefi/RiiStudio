#pragma once

#include <array>
#include <bit>
#include <core/common.h> // u16
#include <glm/vec3.hpp>  // glm::vec3
#include <span>

namespace librii::kmp {

enum class StartPosition { Standard, Near };
enum class Corner { Left, Right };

class Stage {
public:
  bool operator==(const Stage&) const = default;

  u8 mLapCount;
  Corner mCorner;
  StartPosition mStartPosition;
  u8 mFlareTobi; //!< If the flash "jumps"
  struct lensFlareOptions_t {
    bool operator==(const lensFlareOptions_t&) const = default;
    u8 a, r, g, b;
  } mLensFlareOptions;

  // --- Pre Revision 2320: End of Structure

  u8 mUnk08;
  u8 _;
  u16 mSpeedModifier; //!< Used by the speed modifier cheat code as the
                      //!< two most significant bytes of a f32. (Originally
                      //!< implicit pad)
};

#ifdef _WIN32
#define BITCAST_CONSTEXPR constexpr
#else
#define BITCAST_CONSTEXPR
#endif

BITCAST_CONSTEXPR
inline f32 DecodeTruncatedBigFloat(u16 sig) {
  // Sig holds the two most significant bytes of the float in big-endian
  const u8 little_float[] = {0, 0, static_cast<u8>(sig & 0xff),
                             static_cast<u8>(sig >> 8)};

// No std::bit_cast support yet
#ifndef _WIN32
  f32 res;
  memcpy(&res, &little_float[0], 4);
  return res;
#else
  return std::bit_cast<f32>(little_float);
#endif
}

BITCAST_CONSTEXPR inline u16 EncodeTruncatedBigFloat(f32 fl) {
  // No std::bit_cast support yet
#ifndef _WIN32
  std::array<u8, 4> little_float;
  memcpy(little_float.data(), &fl, 4);
#else
  // Sig are the two most significant bytes of the float in big-endian
  const auto little_float = std::bit_cast<std::array<u8, 4>>(fl);
#endif

  return (little_float[3] << 8) | little_float[2];
}

#ifdef _WIN32
static_assert(DecodeTruncatedBigFloat(EncodeTruncatedBigFloat(1.0f)) == 1.0f);
#endif

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
