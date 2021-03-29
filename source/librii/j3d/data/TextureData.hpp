#pragma once

#include <core/common.h>
#include <librii/gx.h>
#include <string>
#include <vector>

namespace librii::j3d {

// Amusingly EGG uses this JUT-named enum directly for BTI
enum class JUTTransparency { Opaque, Clip, Translucent };

struct TextureData {
  std::string mName; // For linking

  librii::gx::TextureFormat mFormat = librii::gx::TextureFormat::I4;
  JUTTransparency transparency = JUTTransparency::Opaque;
  u16 mWidth = 32;
  u16 mHeight = 32;

  u8 mPaletteFormat;
  // Not resolved or supported currently
  u16 nPalette;
  u32 ofsPalette;

  s8 mMinLod;
  s8 mMaxLod;
  u8 mImageCount = 1;

  std::vector<u8> mData = std::vector<u8>(
      librii::gx::computeImageSize(mWidth, mHeight, mFormat, mImageCount));

  bool operator==(const TextureData&) const = default;
};

} // namespace librii::j3d
