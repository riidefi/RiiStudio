#pragma once

#include "plugins/gc/Export/Material.hpp"

#include <core/common.h>
#include <librii/gx.h>
#include <string>
#include <vector>

#include <rsl/SafeReader.hpp>

namespace librii::j3d {

// Amusingly EGG uses this JUT-named enum directly for BTI
enum class JUTTransparency {
  Opaque,
  Clip,
  Translucent,
  // Mario.bdl has this
  MSVC_UNITIALIZED_HEAP_VALUE = 0xCC,
};

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

struct Tex {
  librii::gx::TextureFormat mFormat;
  JUTTransparency transparency;
  u16 mWidth, mHeight;
  librii::gx::TextureWrapMode mWrapU, mWrapV;
  u8 mPaletteFormat;
  u16 nPalette;
  u32 ofsPalette = 0;
  u8 bMipMap;
  u8 bEdgeLod;
  u8 bBiasClamp;
  librii::gx::AnisotropyLevel mMaxAniso;
  librii::gx::TextureFilter mMinFilter;
  librii::gx::TextureFilter mMagFilter;
  s8 mMinLod;
  s8 mMaxLod;
  u8 mMipmapLevel;
  s16 mLodBias;
  u32 ofsTex = 0;

  // Not written, tracked
  s32 btiId = -1;

  bool operator==(const Tex& rhs) const {
    return mFormat == rhs.mFormat && transparency == rhs.transparency &&
           mWidth == rhs.mWidth && mHeight == rhs.mHeight &&
           mWrapU == rhs.mWrapU && mWrapV == rhs.mWrapV &&
           mPaletteFormat == rhs.mPaletteFormat && nPalette == rhs.nPalette &&
           ofsPalette == rhs.ofsPalette && bMipMap == rhs.bMipMap &&
           bEdgeLod == rhs.bEdgeLod && bBiasClamp == rhs.bBiasClamp &&
           mMaxAniso == rhs.mMaxAniso && mMinFilter == rhs.mMinFilter &&
           mMagFilter == rhs.mMagFilter && mMinLod == rhs.mMinLod &&
           mMaxLod == rhs.mMaxLod && mMipmapLevel == rhs.mMipmapLevel &&
           mLodBias == rhs.mLodBias && btiId == rhs.btiId; // ofsTex not checked
  }

  [[nodiscard]] Result<void> transfer(rsl::SafeReader& stream);
  void write(oishii::Writer& stream) const;
  Tex() = default;
  Tex(const librii::j3d::TextureData& data,
      const libcube::GCMaterialData::SamplerData& sampler);
};

} // namespace librii::j3d
