#include "ImportTexture.hpp"

#include <plugins/gc/Export/Texture.hpp>
#include <vendor/stb_image.h>

namespace riistudio::ass {

bool importTexture(libcube::Texture& data, u8* image, std::vector<u8>& scratch,
                   bool mip_gen, int min_dim, int max_mip, int width,
                   int height, int channels) {
  if (!image) {
    data.setWidth(0);
    data.setHeight(0);
    return false;
  }
  assert(image);

  int num_mip = 0;
  if (mip_gen && is_power_of_2(width) && is_power_of_2(height)) {
    while ((num_mip + 1) < max_mip && (width >> (num_mip + 1)) >= min_dim &&
           (height >> (num_mip + 1)) >= min_dim)
      ++num_mip;
  }
  data.setTextureFormat(librii::gx::TextureFormat::CMPR);
  data.setWidth(width);
  data.setHeight(height);
  data.setMipmapCount(num_mip);
  data.setLod(false, 0.0f, static_cast<f32>(data.getImageCount()));
  data.resizeData();
  if (num_mip == 0) {
    data.encode(image);
  } else {
    printf("Width: %u, Height: %u.\n", (unsigned)width, (unsigned)height);
    u32 size = 0;
    for (int i = 0; i <= num_mip; ++i) {
      size += (width >> i) * (height >> i) * 4;
      printf("Image %i: %u, %u. -> Total Size: %u\n", i, (unsigned)(width >> i),
             (unsigned)(height >> i), size);
    }
    scratch.resize(size);

    u32 slide = 0;
    for (int i = 0; i <= num_mip; ++i) {
      librii::image::resize(scratch.data() + slide, width >> i, height >> i,
                            image, width, height, librii::image::Lanczos);
      slide += (width >> i) * (height >> i) * 4;
    }

    data.encode(scratch.data());
  }
  stbi_image_free(image);
  return true;
}
bool importTexture(libcube::Texture& data, const u8* idata, const u32 isize,
                   std::vector<u8>& scratch, bool mip_gen, int min_dim,
                   int max_mip) {
  int width, height, channels;
  u8* image = stbi_load_from_memory(idata, isize, &width, &height, &channels,
                                    STBI_rgb_alpha);
  return importTexture(data, image, scratch, mip_gen, min_dim, max_mip, width,
                       height, channels);
}
bool importTexture(libcube::Texture& data, const char* path,
                   std::vector<u8>& scratch, bool mip_gen, int min_dim,
                   int max_mip) {
  int width, height, channels;
  u8* image = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
  return importTexture(data, image, scratch, mip_gen, min_dim, max_mip, width,
                       height, channels);
}

} // namespace riistudio::ass
