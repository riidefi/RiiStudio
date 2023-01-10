#pragma once

#include <string>
#include <vector>

#include <core/common.h>

#include <librii/g3d/data/TextureData.hpp>
#include <plugins/gc/Export/Texture.hpp>

namespace riistudio::g3d {

struct Texture : public librii::g3d::TextureData,
                 public libcube::Texture,
                 public virtual kpi::IObject {
  std::string getName() const override { return name; }
  void setName(const std::string& n) override { name = n; }
  librii::gx::TextureFormat getTextureFormat() const override { return format; }
  void setTextureFormat(librii::gx::TextureFormat f) override { format = f; }
  u32 getImageCount() const override { return number_of_images; }
  void setImageCount(u32 c) override { number_of_images = c; }
  std::span<const u8> getData() const override { return data; }
  std::span<u8> getData() override { return data; }
  void resizeData() override { data.resize(getEncodedSize(true)); }
  const u8* getPaletteData() const override { return nullptr; }
  u32 getPaletteFormat() const override { return 0; }
  u16 getWidth() const override { return width; }
  void setWidth(u16 w) override { width = w; }
  u16 getHeight() const override { return height; }
  void setHeight(u16 h) override { height = h; }
  void setLod(bool custom, f32 min_, f32 max_) override {
    custom_lod = custom;
    minLod = min_;
    maxLod = max_;
  }
  void setSourcePath(std::string_view path) override {
    sourcePath = std::string(path);
  }
  f32 getMinLod() const override { return minLod; }
  f32 getMaxLod() const override { return maxLod; }
  std::string getSourcePath() const override { return sourcePath; }

  bool operator==(const Texture& rhs) const {
    return TextureData::operator==(static_cast<const TextureData&>(rhs));
  }
};

} // namespace riistudio::g3d
