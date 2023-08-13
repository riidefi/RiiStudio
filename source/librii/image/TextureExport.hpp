#pragma once

#include <expected>
#include <string>

namespace librii {

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

} // namespace librii
