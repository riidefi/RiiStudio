#pragma once

#include <core/common.h>

namespace rsl::stb {

struct ImageResult {
  std::vector<u8> data;
  int width{};
  int height{};
  int channels{};
};

[[nodiscard]] Result<ImageResult> load_from_memory(std::span<const u8> view);
[[nodiscard]] Result<ImageResult> load(std::string_view path);

// Import from librii
// TODO: Rename
enum class STBImage {
  PNG,
  BMP,
  TGA,
  JPG,
  HDR,
};

// 8 bit data -- For HDR pass f32s
[[nodiscard]] std::expected<void, std::string>
writeImageStb(const char* filename, STBImage type, int x, int y,
              int channel_component_count, const void* data);

[[nodiscard]] static inline std::expected<void, std::string>
writeImageStbRGBA(const char* filename, STBImage type, int x, int y,
                  const void* data) {
  return writeImageStb(filename, type, x, y, 4, data);
}
[[nodiscard]] static inline std::expected<void, std::string>
writeImageStbRGB(const char* filename, STBImage type, int x, int y,
                 const void* data) {
  return writeImageStb(filename, type, x, y, 3, data);
}

} // namespace rsl::stb
