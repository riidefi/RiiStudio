#pragma once

#include <core/common.h>
#include <glm/vec3.hpp>
#include <optional>
#include <span>
#include <vector>

namespace librii::image {

// analysis.c

//! Get the average 32-bit color from a range
u32 ComputeAverageColor(std::span<const u32> colors);

glm::vec3 ComputePrincipalComponent(std::span<const u32> colors);

// texconv.c

enum class DitherMode { None, Color, ColorAlpha };

enum class DsFormat {
  DS_A3I5 = 1,
  DS_4COLOR = 2,
  DS_16COLOR = 3,
  DS_256COLOR = 4,
  DS_4x4 = 5,
  DS_A5I3 = 6,
  DS_DIRECT = 7,
};

struct DsTexture {
  std::vector<u8> pixel_data;
  std::vector<u16> index_data;
  int ds_param; // Some Garhoogin value
};
struct DsPalette {
  std::vector<u16> color_data;
};

struct DsTexturePalette {
  DsTexture texture;
  DsPalette palette;
};

//! Convert 8-bit RGBA colors to a DS format
std::optional<DsTexturePalette>
ConvertToDS(std::span<const u32> pixels, u32 width, u32 height, DsFormat format,
            DitherMode dither, u32 palette_size,
            std::optional<std::span<const u32>> custom_palette = std::nullopt,
            float yuv_merge_threshhold = 0.0f);

} // namespace librii::image
