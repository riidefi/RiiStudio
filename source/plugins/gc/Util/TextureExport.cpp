#include <plugins/gc/Util/TextureExport.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <vendor/stb_image_write.h>

namespace libcube {

void writeImageStb(const char *filename, STBImage type, int x, int y,
                   int channel_component_count, const void *data) {
  switch (type) {
  case STBImage::PNG:
    stbi_write_png(filename, x, y, channel_component_count, data,
                   x * channel_component_count);
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
                   static_cast<const float *>(data));
    break;
  default:
    assert(!"Invalid format.");
    break;
  }
}

} // namespace libcube
