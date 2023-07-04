#include "Texture.hpp"

#include "gctex/include/gctex.h"

namespace librii::gx {

u32 computeImageSize(u16 width, u16 height, u32 format, u8 number_of_images) {
  return rii_compute_image_size_mip(format, width, height, number_of_images);
}

} // namespace librii::gx
