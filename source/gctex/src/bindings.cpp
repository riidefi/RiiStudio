#include "ImagePlatform.hpp"

extern "C" {
void impl_rii_decode(u8* dst, const u8* src, u32 width, u32 height,
                     u32 texformat, const u8* tlut, u32 tlutformat) {
  librii::image::decode(dst, src, width, height, texformat, tlut, tlutformat);
}
}
