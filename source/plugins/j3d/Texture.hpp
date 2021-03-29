#pragma once

#include <core/common.h>
#include <librii/gx.h>
#include <librii/j3d/data/TextureData.hpp>
#include <plugins/gc/Export/Texture.hpp>
#include <vector>
#include <vendor/fa5/IconsFontAwesome5.h>

namespace riistudio::j3d {

struct Texture : public librii::j3d::TextureData, public libcube::Texture {
  // PX_TYPE_INFO_EX("J3D Texture", "j3d_tex", "J::Texture", ICON_FA_IMAGES,
  // ICON_FA_IMAGE);

  std::string getName() const override { return mName; }
  void setName(const std::string& name) override { mName = name; }

  librii::gx::TextureFormat getTextureFormat() const override {
    return mFormat;
  }

  void setTextureFormat(librii::gx::TextureFormat format) override {
    mFormat = format;
  }
  u32 getImageCount() const override { return mImageCount; }
  void setImageCount(u32 c) override { mImageCount = c; }
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
