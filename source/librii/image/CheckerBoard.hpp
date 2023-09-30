#pragma once

#include <array>
#include <plugins/3d/Texture.hpp>
#include <core/common.h>
#include <span>

namespace librii::image {

static inline constexpr void generateCheckerboard(std::span<u8> scratch,
                                                  int width, int height,
                                                  u32 fg = 0xff'00'dc'ff,
                                                  u32 bg = 0x00'00'00'ff) {
  for (int i = 0; i < height; ++i) {
    for (int j = 0; j < width; ++j) {
      const auto px_offset = i * width * 4 + j * 4;
      const bool is_fg = ((i / 8) & 1) != ((j / 8) & 1);
      const u32 color = is_fg ? fg : bg;
      scratch[px_offset] = (color & 0xff'00'00'00) >> 24;
      scratch[px_offset + 1] = (color & 0x00'ff'00'00) >> 16;
      scratch[px_offset + 2] = (color & 0x00'00'ff'00) >> 8;
      scratch[px_offset + 3] = (color & 0x00'00'00'ff);
    }
  }
}

template <u32 W, u32 H> struct NullTextureData {
  std::array<u8, W * H * 4> pixels_rgba32raw;
  u32 width;
  u32 height;

  constexpr NullTextureData() : width(W), height(H) {
    librii::image::generateCheckerboard(pixels_rgba32raw, width, height);
  }
};

template <u32 W, u32 H> struct NullTexture : public riistudio::lib3d::Texture {
  const NullTextureData<W, H>& data;
  NullTexture(const NullTextureData<W, H>& data_) : data(data_) {}

  u32 getEncodedSize(bool mip) const override {
    return data.pixels_rgba32raw.size();
  }
  Result<void> decode(std::vector<u8>& out, bool mip) const override {
    out.insert(out.begin(), data.pixels_rgba32raw.begin(),
               data.pixels_rgba32raw.end());
    return {};
  }

  u32 getImageCount() const override { return 1; }

  u16 getWidth() const override { return data.width; }
  u16 getHeight() const override { return data.height; }

  // Unimplemented
  void setName(const std::string& name) override {}
  void setImageCount(u32 c) override {}
  void setWidth(u16 width) override {}
  void setHeight(u16 height) override {}
  void setEncoder(bool optimizeForSize, bool color,
                  Occlusion occlusion) override {}
  Result<void> encode(std::span<const u8> rawRGBA) override { return {}; }
};

} // namespace librii::image
