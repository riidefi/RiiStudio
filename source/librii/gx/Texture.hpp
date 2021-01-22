#pragma once

#include <core/common.h>

namespace librii::gx {

enum class AnisotropyLevel { x1, x2, x4 };

enum {
  _CTF = 0x20,
  _ZTF = 0x10,
};
enum class TextureFormat {
  I4,
  I8,
  IA4,
  IA8,
  RGB565,
  RGB5A3,
  RGBA8,

  C4,
  C8,
  C14X2,
  CMPR = 0xE,

  Extension_RawRGBA32 = 0xFF
};

enum class CopyTextureFormat {
  R4 = 0x0 | _CTF,
  RA4 = 0x2 | _CTF,
  RA8 = 0x3 | _CTF,
  YUVA8 = 0x6 | _CTF,
  A8 = 0x7 | _CTF,
  R8 = 0x8 | _CTF,
  G8 = 0x9 | _CTF,
  B8 = 0xA | _CTF,
  RG8 = 0xB | _CTF,
  GB8 = 0xC | _CTF
};

enum class ZTextureFormat {
  Z8 = 0x1 | _ZTF,
  Z16 = 0x3 | _ZTF,
  Z24X8 = 0x6 | _ZTF,

  Z4 = 0x0 | _ZTF | _CTF,
  Z8M = 0x9 | _ZTF | _CTF,
  Z8L = 0xA | _ZTF | _CTF,
  Z16L = 0xC | _ZTF | _CTF
};

enum class PaletteFormat { IA8, RGB565, RGB5A3 };
enum class TextureFilter {
  Near,
  Linear,
  near_mip_near,
  lin_mip_near,
  near_mip_lin,
  lin_mip_lin
};

enum class TextureWrapMode { Clamp, Repeat, Mirror };

// I'm not sure I want these here. Perhaps they'd fit better in librii::image
struct ImageFormatInfo {
  u32 xshift = 0;
  u32 yshift = 0;
  u32 bitsize = 0;
};

ImageFormatInfo getFormatInfo(u32 format);

u32 computeImageSize(ImageFormatInfo info, u32 width, u32 height);
u32 computeImageSize(u16 width, u16 height, u32 format, u8 number_of_images);
inline u32 computeImageSize(u16 width, u16 height, TextureFormat format,
                            u8 number_of_images) {
  return computeImageSize(width, height, static_cast<u32>(format),
                          number_of_images);
}

} // namespace librii::gx
