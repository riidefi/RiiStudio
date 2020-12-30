#pragma once

namespace librii::gx {
	
enum class AnisotropyLevel { x1, x2, x4 };

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
enum class PaletteFormat { IA8, RGB565, RGB5A3 };
enum class TextureFilter {
#undef near
  near,
  linear,
  near_mip_near,
  lin_mip_near,
  near_mip_lin,
  lin_mip_lin
};

enum class TextureWrapMode { Clamp, Repeat, Mirror };
	
} // namepace librii::gx