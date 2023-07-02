#include "CmprEncoder.hpp"

#include "gctex/include/gctex.h"

namespace librii::image {

void EncodeDXT1(u8* dest_img, const u8* source_img, u32 width, u32 height) {
  rii_encode_cmpr(dest_img, 0, source_img, 0, width, height);
}

} // namespace librii::image
