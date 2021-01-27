#pragma once

#include <librii/gx.h>

namespace librii::hx {

enum class TextureFilterMode : int { Near, Linear };

//! For minification filter only: maginification does not support mipmap types
struct TextureFilter {
  int minBase = 0;    // TextureFilterMode
  int minMipBase = 0; // Mipmap filter: TextureFilterMode
  bool mip = false;   // If mipmapping is used; lin_mip_lin vs lin

  bool operator==(const TextureFilter&) const = default;
};

inline TextureFilter elevateTextureFilter(gx::TextureFilter filter) {
  TextureFilter result;

  switch (filter) {
  case gx::TextureFilter::near_mip_near:
    result.mip = true;
    // Fallthrough
  case gx::TextureFilter::Near:
    result.minBase = result.minMipBase =
        static_cast<int>(gx::TextureFilter::Near);
    break;
  case gx::TextureFilter::lin_mip_lin:
    result.mip = true;
    // Fallthrough
  case gx::TextureFilter::Linear:
    result.minBase = result.minMipBase =
        static_cast<int>(gx::TextureFilter::Linear);
    break;
  case gx::TextureFilter::near_mip_lin:
    result.mip = true;
    result.minBase = static_cast<int>(gx::TextureFilter::Near);
    result.minMipBase = static_cast<int>(gx::TextureFilter::Linear);
    break;
  case gx::TextureFilter::lin_mip_near:
    result.mip = true;
    result.minBase = static_cast<int>(gx::TextureFilter::Linear);
    result.minMipBase = static_cast<int>(gx::TextureFilter::Near);
    break;
  }

  return result;
}

inline gx::TextureFilter lowerTextureFilter(TextureFilter filter) {
  if (!filter.mip)
    return static_cast<gx::TextureFilter>(filter.minBase);

  bool baseLin = static_cast<gx::TextureFilter>(filter.minBase) ==
                 gx::TextureFilter::Linear;
  if (static_cast<gx::TextureFilter>(filter.minMipBase) ==
      gx::TextureFilter::Linear) {
    return baseLin ? gx::TextureFilter::lin_mip_lin
                   : gx::TextureFilter::near_mip_lin;
  } else {
    return baseLin ? gx::TextureFilter::lin_mip_near
                   : gx::TextureFilter::near_mip_near;
  }
}

} // namespace librii::hx
