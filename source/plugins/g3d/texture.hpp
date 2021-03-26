#pragma once

#include <string>
#include <vector>

#include <core/common.h>

#include <plugins/gc/Export/Texture.hpp>

namespace riistudio::g3d {

struct TextureData {
  std::string name{"Untitled"};
  librii::gx::TextureFormat format = librii::gx::TextureFormat::I4;
  u32 width = 32;
  u32 height = 32;

  u32 mipLevel{1}; // 1 - no mipmaps
  bool custom_lod = false;
  f32 minLod{0.0f};
  f32 maxLod{1.0f};

  std::string sourcePath;
  std::vector<u8> data =
      std::vector<u8>(librii::gx::computeImageSize(width, height, format, 1));

  bool operator==(const TextureData& rhs) const = default;
};

struct Texture : public TextureData,
                 public libcube::Texture,
                 public virtual kpi::IObject {
  std::string getName() const override { return name; }
  void setName(const std::string& n) override { name = n; }
  librii::gx::TextureFormat getTextureFormat() const override { return format; }
  void setTextureFormat(librii::gx::TextureFormat f) override { format = f; }
  u32 getMipmapCount() const override {
    assert(mipLevel > 0);
    return mipLevel - 1;
  }
  void setMipmapCount(u32 c) override { mipLevel = c + 1; }
  const u8* getData() const override { return data.data(); }
  u8* getData() override { return data.data(); }
  void resizeData() override { data.resize(getEncodedSize(true)); }
  const u8* getPaletteData() const override { return nullptr; }
  u32 getPaletteFormat() const override { return 0; }
  u16 getWidth() const override { return width; }
  void setWidth(u16 w) override { width = w; }
  u16 getHeight() const override { return height; }
  void setHeight(u16 h) override { height = h; }

  bool operator==(const Texture& rhs) const {
    return TextureData::operator==(static_cast<const TextureData&>(rhs));
  }
};

} // namespace riistudio::g3d
