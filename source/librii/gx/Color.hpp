#pragma once

#include <algorithm>
#include <core/common.h>
#include <glm/glm.hpp>

namespace librii::gx {

struct Color;
struct ColorS10;
struct ColorF32 {
  f32 r, g, b, a;

  inline operator float*() { return &r; }
  operator glm::vec4() const { return {r, g, b, a}; }

  inline void clamp(f32 min, f32 max) {
#undef min
#undef max
    auto clampEach = [&](auto r) { return std::max(std::min(r, max), min); };
    r = clampEach(r);
    g = clampEach(g);
    b = clampEach(b);
    a = clampEach(a);
  }

  operator Color() const;
  operator ColorS10() const;

  inline bool operator==(const ColorF32& rhs) const = default;
  inline bool operator!=(const ColorF32& rhs) const = default;
};
struct Color {
  u32 r = 0, g = 0, b = 0, a = 0;

  inline bool operator==(const Color& rhs) const noexcept = default;

  inline operator ColorF32() const {
    return {static_cast<float>(r) / static_cast<float>(0xff),
            static_cast<float>(g) / static_cast<float>(0xff),
            static_cast<float>(b) / static_cast<float>(0xff),
            static_cast<float>(a) / static_cast<float>(0xff)};
  }
  inline Color() : r(0), g(0), b(0), a(0) {}
  inline Color(u32 hex) {
    r = (hex & 0xff000000) << 24;
    g = (hex & 0x00ff0000) << 16;
    b = (hex & 0x0000ff00) << 8;
    a = (hex & 0x000000ff);
  }
  inline Color(u8 _r, u8 _g, u8 _b, u8 _a) : r(_r), g(_g), b(_b), a(_a) {}
};

struct ColorS10 {
  s32 r = 0, g = 0, b = 0, a = 0;

  bool operator==(const ColorS10& rhs) const = default;
  inline operator ColorF32() const {
    return {static_cast<float>(r) / static_cast<float>(0xff),
            static_cast<float>(g) / static_cast<float>(0xff),
            static_cast<float>(b) / static_cast<float>(0xff),
            static_cast<float>(a) / static_cast<float>(0xff)};
  }
};
inline ColorF32::operator Color() const {
  return {(u8)roundf(r * 255.0f), (u8)roundf(g * 255.0f),
          (u8)roundf(b * 255.0f), (u8)roundf(a * 255.0f)};
}
inline ColorF32::operator ColorS10() const {
  return {(s16)roundf(r * 255.0f), (s16)roundf(g * 255.0f),
          (s16)roundf(b * 255.0f), (s16)roundf(a * 255.0f)};
}

} // namespace librii::gx
