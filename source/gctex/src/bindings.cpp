#include "CmprEncoder.hpp"
#include "ImagePlatform.hpp"

extern "C" {
void impl_rii_decode(u8* dst, const u8* src, u32 width, u32 height,
                     u32 texformat,
                const u8* tlut, u32 tlutformat) {
  librii::image::decode(dst, src, width, height, texformat, tlut, tlutformat);
}
void impl_rii_encodeCMPR(u8* dest_img, const u8* source_img, u32 width,
                         u32 height) {
  return librii::image::EncodeDXT1(dest_img, source_img, width, height);
}
void impl_rii_encodeI4(u8* dst, const u32* src, u32 width, u32 height) {
  return librii::image::encodeI4(dst, src, width, height);
}
void impl_rii_encodeI8(u8* dst, const u32* src, u32 width, u32 height) {
  return librii::image::encodeI8(dst, src, width, height);
}
void impl_rii_encodeIA4(u8* dst, const u32* src, u32 width, u32 height) {
  return librii::image::encodeIA4(dst, src, width, height);
}
void impl_rii_encodeIA8(u8* dst, const u32* src, u32 width, u32 height) {
  return librii::image::encodeIA8(dst, src, width, height);
}
void impl_rii_encodeRGB565(u8* dst, const u32* src, u32 width, u32 height) {
  return librii::image::encodeRGB565(dst, src, width, height);
}
void impl_rii_encodeRGB5A3(u8* dst, const u32* src, u32 width, u32 height) {
  return librii::image::encodeRGB5A3(dst, src, width, height);
}
void impl_rii_encodeRGBA8(u8* dst, const u32* src, u32 width, u32 height) {
  return librii::image::encodeRGBA8(dst, src, width, height);
}
}
