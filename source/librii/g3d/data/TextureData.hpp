#pragma once

#include <core/common.h>
#include <librii/gx.h>
#include <string>
#include <vector>

namespace librii::g3d {

struct TextureData {
  std::string name{"Untitled"};
  librii::gx::TextureFormat format = librii::gx::TextureFormat::I4;
  u32 width = 32;
  u32 height = 32;

  u32 number_of_images{1}; // 1 - no mipmaps
  bool custom_lod = false; // If false, minLod and maxLod are recalculated.
  f32 minLod{0.0f};
  f32 maxLod{1.0f};

  std::string sourcePath;
  std::vector<u8> data = std::vector<u8>(
      librii::gx::computeImageSize(width, height, format, number_of_images));

  bool operator==(const TextureData& rhs) const = default;
};

inline u32 ComputeImageSize(const TextureData& tex) {
  return librii::gx::computeImageSize(tex.width, tex.height, tex.format,
                                      tex.number_of_images);
}

} // namespace librii::g3d
