#include "Stb.hpp"

#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <vendor/stb_image_write.h>

#include "WriteFile.hpp"

namespace rsl::stb {

using STB_DELETER = decltype([](u8* x) { stbi_image_free(x); });
using STB_PTR = std::unique_ptr<u8, STB_DELETER>;

Result<ImageResult> load_from_memory(std::span<const u8> view) {
  int width = 0;
  int height = 0;
  int channels = 0;
  STB_PTR data{stbi_load_from_memory(view.data(), view.size_bytes(), &width,
                                     &height, &channels, STBI_rgb_alpha)};
  if (data == nullptr) {
    return std::unexpected("stbi_load_from_memory failed");
  }

  int encoded_size = width * height * 4;

  return ImageResult{
      .data = {data.get(), data.get() + encoded_size},
      .width = width,
      .height = height,
      .channels = channels,
  };
}
Result<ImageResult> load(std::string_view path) {
  int width = 0;
  int height = 0;
  int channels = 0;
  std::string path_(path);
  STB_PTR data{
      stbi_load(path_.c_str(), &width, &height, &channels, STBI_rgb_alpha)};
  if (data == nullptr) {
    return std::unexpected("stbi_load failed");
  }

  int encoded_size = width * height * 4;

  return ImageResult{
      .data = {data.get(), data.get() + encoded_size},
      .width = width,
      .height = height,
      .channels = channels,
  };
}

Result<void> writeImageStb(const char* filename, STBImage type, int x, int y,
                           int channel_component_count, const void* data) {
  int len = 0;
  unsigned char* encoded = nullptr;

  // For now, we'll only save png files from web...
  switch (type) {
  case STBImage::PNG:
    encoded = stbi_write_png_to_mem(
        reinterpret_cast<const unsigned char*>(data),
        x * channel_component_count, x, y, channel_component_count, &len);
    TRY(rsl::WriteFile(
        std::span<uint8_t>{encoded, static_cast<std::size_t>(len)}, filename));
    break;
  case STBImage::BMP:
    stbi_write_bmp(filename, x, y, channel_component_count, data);
    break;
  case STBImage::TGA:
    stbi_write_tga(filename, x, y, channel_component_count, data);
    break;
  case STBImage::JPG:
    stbi_write_jpg(filename, x, y, channel_component_count, data, 100);
    break;
  case STBImage::HDR:
    stbi_write_hdr(filename, x, y, channel_component_count,
                   static_cast<const float*>(data));
    break;
  }
  return std::unexpected("Invalid format.");
}

} // namespace rsl::stb
