#include "TextureExport.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <vendor/stb_image_write.h>

#include <plate/Platform.hpp>

namespace librii {

void writeImageStb(const char* filename, STBImage type, int x, int y,
                   int channel_component_count, const void* data) {
  int len = 0;
  unsigned char* encoded = nullptr;

  // For now, we'll only save png files from web...
  switch (type) {
  case STBImage::PNG:
    encoded = stbi_write_png_to_mem(
        reinterpret_cast<const unsigned char*>(data),
        x * channel_component_count, x, y, channel_component_count, &len);
    plate::Platform::writeFile(
        std::span<uint8_t>{encoded, static_cast<std::size_t>(len)}, filename);
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
  default:
    assert(!"Invalid format.");
    break;
  }
}

} // namespace librii
