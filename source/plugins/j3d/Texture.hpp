#pragma once

#include <core/common.h>
#include <lib_rii/gx.h>
#include <vector>

#include <plugins/gc/Export/Texture.hpp>

#include <vendor/fa5/IconsFontAwesome5.h>

namespace riistudio::j3d {

struct TextureData {
  std::string mName; // For linking

  u8 mFormat = 0;
  bool bTransparent = false;
  u16 mWidth = 32, mHeight = 32;

  u8 mPaletteFormat;
  // Not resolved or supported currently
  u16 nPalette;
  u32 ofsPalette;

  s8 mMinLod;
  s8 mMaxLod;
  u8 mMipmapLevel = 1;

  std::vector<u8> mData =
      std::vector<u8>(GetTexBufferSize(mWidth, mHeight, mFormat, 0, 0));

  bool operator==(const TextureData& rhs) const = default;
};

struct Texture : public TextureData, public libcube::Texture {
  // PX_TYPE_INFO_EX("J3D Texture", "j3d_tex", "J::Texture", ICON_FA_IMAGES,
  // ICON_FA_IMAGE);

  std::string getName() const override { return mName; }
  void setName(const std::string& name) override { mName = name; }

  u32 getTextureFormat() const override { return mFormat; }

  void setTextureFormat(u32 format) override { mFormat = format; }
  u32 getMipmapCount() const override {
    assert(mMipmapLevel > 0);
    return mMipmapLevel - 1;
  }
  void setMipmapCount(u32 c) override { mMipmapLevel = c + 1; }
  const u8* getData() const override { return mData.data(); }
  u8* getData() override { return mData.data(); }
  void resizeData() override { mData.resize(getEncodedSize(true)); }

  const u8* getPaletteData() const override { return nullptr; }
  u32 getPaletteFormat() const override { return 0; }

  u16 getWidth() const override { return mWidth; }
  void setWidth(u16 width) override { mWidth = width; }
  u16 getHeight() const override { return mHeight; }
  void setHeight(u16 height) override { mHeight = height; }
};

} // namespace riistudio::j3d
