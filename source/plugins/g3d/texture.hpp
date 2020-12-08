#pragma once

#include <string>
#include <vector>

#include <core/3d/texture_dimensions.hpp>
#include <core/common.h>

#include <plugins/gc/Export/Texture.hpp>

namespace riistudio::g3d {

struct TextureData {
  std::string name{"Untitled"};
  u32 format = 0;
  core::TextureDimensions<u16> dimensions{32, 32};

  u32 mipLevel{1}; // 1 - no mipmaps
  f32 minLod{0.0f};
  f32 maxLod{1.0f};

  std::string sourcePath;
  std::vector<u8> data = std::vector<u8>(
      GetTexBufferSize(dimensions.width, dimensions.height, format, 0, 0));

  bool operator==(const TextureData& rhs) const = default;
};

struct Texture : public TextureData,
                 public libcube::Texture,
                 public virtual kpi::IObject {
  std::string getName() const override { return name; }
  void setName(const std::string& n) override { name = n; }
  u32 getTextureFormat() const override { return (u32)format; }
  void setTextureFormat(u32 f) override { format = f; }
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
  u16 getWidth() const override { return dimensions.width; }
  void setWidth(u16 w) override { dimensions.width = w; }
  u16 getHeight() const override { return dimensions.height; }
  void setHeight(u16 h) override { dimensions.height = h; }

  bool operator==(const Texture& rhs) const {
    return TextureData::operator==(static_cast<const TextureData&>(rhs));
  }
};

} // namespace riistudio::g3d
