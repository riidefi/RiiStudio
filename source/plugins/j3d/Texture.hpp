#pragma once

#include <core/common.h>
#include <core/util/timestamp.hpp>
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
  std::span<const u8> getData() const override { return mData; }
  std::span<u8> getData() override { return mData; }
  void resizeData() override { mData.resize(getEncodedSize(true)); }

  const u8* getPaletteData() const override { return nullptr; }
  u32 getPaletteFormat() const override { return 0; }

  u16 getWidth() const override { return mWidth; }
  void setWidth(u16 width) override { mWidth = width; }
  u16 getHeight() const override { return mHeight; }
  void setHeight(u16 height) override { mHeight = height; }

  void setLod(bool custom, f32 min_, f32 max_) override {
    mMinLod = min_;
    mMaxLod = max_;
  }
  void setSourcePath(std::string_view) override {}
  f32 getMinLod() const override { return mMinLod; }
  f32 getMaxLod() const override { return mMaxLod; }
  std::string getSourcePath() const override {
    return std::string(RII_TIME_STAMP) + "; J3D Export";
  }
};

} // namespace riistudio::j3d
