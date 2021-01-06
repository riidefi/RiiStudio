#include "Texture.hpp"

namespace librii::gx {

ImageFormatInfo getFormatInfo(u32 format) {
  u32 xshift = 0;
  u32 yshift = 0;
  switch (format) {
  case (u32)TextureFormat::C4:
  case (u32)TextureFormat::I4:
  case (u32)TextureFormat::CMPR:
  case (u32)CopyTextureFormat::R4:
  case (u32)CopyTextureFormat::RA4:
  case (u32)ZTextureFormat::Z4:
    xshift = 3;
    yshift = 3;
    break;
  case (u32)ZTextureFormat::Z8:
  case (u32)TextureFormat::C8:
  case (u32)TextureFormat::I8:
  case (u32)TextureFormat::IA4:
  case (u32)CopyTextureFormat::A8:
  case (u32)CopyTextureFormat::R8:
  case (u32)CopyTextureFormat::G8:
  case (u32)CopyTextureFormat::B8:
  case (u32)CopyTextureFormat::RG8:
  case (u32)CopyTextureFormat::GB8:
  case (u32)ZTextureFormat::Z8M:
  case (u32)ZTextureFormat::Z8L:
    xshift = 3;
    yshift = 2;
    break;
  case (u32)TextureFormat::C14X2:
  case (u32)TextureFormat::IA8:
  case (u32)ZTextureFormat::Z16:
  case (u32)ZTextureFormat::Z24X8:
  case (u32)TextureFormat::RGB565:
  case (u32)TextureFormat::RGB5A3:
  case (u32)TextureFormat::RGBA8:
  case (u32)ZTextureFormat::Z16L:
  case (u32)CopyTextureFormat::RA8:
    xshift = 2;
    yshift = 2;
    break;
  default:
    assert(!"Invalid texture format");
    xshift = 2;
    yshift = 2;
    break;
  }

  u32 bitsize = 32;
  if (format == (u32)TextureFormat::RGBA8 ||
      format == (u32)ZTextureFormat::Z24X8)
    bitsize = 64;

  return {xshift, yshift, bitsize};
}

u32 computeImageSize(ImageFormatInfo info, u32 width, u32 height) {
  const u32 xtiles = ((width + (1 << info.xshift)) - 1) >> info.xshift;
  const u32 ytiles = ((height + (1 << info.yshift)) - 1) >> info.yshift;

  return xtiles * ytiles * info.bitsize;
}

u32 computeImageSize(u16 width, u16 height, u32 format, u8 number_of_images) {
  // Extension_RawRGBA32
  if (format == 0xFF) {
    if (number_of_images <= 1)
      return width * height * 4;

    u32 size = 0;
    for (u32 i = 0; i < number_of_images; ++i) {
      if (width == 1 && height == 1)
        break;

      size += width * height * 4;

      if (width > 1)
        width >>= 1;
      else
        width = 1;

      if (height > 1)
        height >>= 1;
      else
        height = 1;
    }

    return size;
  }

  const auto info = getFormatInfo(format);

  if (number_of_images <= 1)
    return computeImageSize(info, width, height);

  u32 size = 0;

  for (u32 i = 0; i < number_of_images; ++i) {
    if (width == 1 && height == 1)
      break;

    size += computeImageSize(info, width, height);

    if (width > 1)
      width >>= 1;
    else
      width = 1;

    if (height > 1)
      height >>= 1;
    else
      height = 1;
  }

  return size;
}

} // namespace librii::gx
