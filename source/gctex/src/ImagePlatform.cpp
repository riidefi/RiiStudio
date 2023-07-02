#include "CmprEncoder.hpp"

#include <string.h>

#include "dolemu/TextureDecoder/TextureDecoder.h"

namespace librii {
namespace image {

#if 0
std::pair<u32, u32> getBlockedDimensions(u32 width, u32 height,
                                         gx::TextureFormat format) {
  const u32 blocksize = format == gx::TextureFormat::RGBA8 ? 64 : 32;

  return {(width + blocksize - 1) & ~(blocksize - 1),
          (height + blocksize - 1) & ~(blocksize - 1)};
}

int getEncodedSize(int width, int height, gx::TextureFormat format,
                   u32 mipMapCount) {
  assert(mipMapCount < 0xff);
  return librii::gx::computeImageSize(width, height, static_cast<u32>(format),
                                      mipMapCount + 1);
}
#endif

// X -> raw 8-bit RGBA
void decode(u8* dst, const u8* src, u32 width, u32 height, u32 texformat,
            const u8* tlut, u32 tlutformat) {
  TexDecoder_Decode(dst, src, static_cast<int>(width), static_cast<int>(height),
                    static_cast<TextureFormat>(texformat), tlut,
                    static_cast<TLUTFormat>(tlutformat));
}

struct rgba {
  u8 r;
  u8 g;
  u8 b;
  u8 a;
};

constexpr u8 luminosity(const rgba& rgba) {
  double f = static_cast<double>(rgba.r) * 0.299 +
             static_cast<double>(rgba.g) * 0.587 +
             static_cast<double>(rgba.b) * 0.144;
  return static_cast<u8>(f);
}

void encodeI4(u8* dst, const u32* src, u32 width, u32 height) {
  for (u32 y = 0; y < height; y += 8) {
    for (u32 x = 0; x < width; x += 8) {
      // Encode the 8x8 block
      // There are 8 rows (4 bits wide) and 4 columns

      for (int row = 0; row < 8; ++row) {
        for (int column = 0; column < 4; ++column) {
          struct rgba_x2 {
            rgba _0;
            rgba _1;
          };
          rgba_x2 rgba = *(rgba_x2*)&src[(y + row) * width + x + column * 2];
          u32 i0 = luminosity(rgba._0);
          u32 i1 = luminosity(rgba._1);

          u8 i0i1 = (i0 & 0b11'11'00'00) | ((i1 >> 4) & 0b11'11);
          dst[column] = i0i1;
        }
        dst += 4;
      }
    }
  }
}
void encodeI8(u8* dst, const u32* src, u32 width, u32 height) {
  for (u32 y = 0; y < height; y += 4) {
    for (u32 x = 0; x < width; x += 8) {
      // Encode the 4x8 block
      // There are 4 rows (8 bits wide) and 4 columns
      // Same block size as I4

      for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 8; ++column) {
          rgba _rgba = *(rgba*)&src[(y + row) * width + x + column];
          dst[column] = luminosity(_rgba);
        }
        dst += 8;
      }
    }
  }
}
void encodeIA4(u8* dst, const u32* src, u32 width, u32 height) {
  for (u32 y = 0; y < height; y += 4) {
    for (u32 x = 0; x < width; x += 8) {
      // Encode the 4x8 block
      // There are 4 rows (8 bits wide) and 4 columns
      // Same block size as I4

      for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 8; ++column) {
          rgba _rgba = *(rgba*)&src[(y + row) * width + x + column];
          dst[column] = (luminosity(_rgba) & 0b11'11'00'00) | (_rgba.a >> 4);
        }
        dst += 8;
      }
    }
  }
}
void encodeIA8(u8* dst, const u32* src, u32 width, u32 height) {
  for (u32 y = 0; y < height; y += 4) {
    for (u32 x = 0; x < width; x += 4) {
      for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
          rgba c = *(rgba*)&src[(y + row) * width + x + column];
          dst[column * 2] = luminosity(c);
          dst[column * 2 + 1] = c.a;
        }
        dst += 8;
      }
    }
  }
}

void encodeRGB565(u8* dst, const u32* src, u32 width, u32 height) {
  for (u32 y = 0; y < height; y += 4) {
    for (u32 x = 0; x < width; x += 4) {
      for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
          rgba c = *(rgba*)&src[(y + row) * width + x + column];
          u16 packed =
              ((c.r & 0xf8) << 8) | ((c.g & 0xfc) << 3) | ((c.b & 0xf8) >> 3);
          dst[column * 2] = packed >> 8;
          dst[column * 2 + 1] = packed & 0xFF;
        }
        dst += 8;
      }
    }
  }
}

void encodeRGB5A3(u8* dst, const u32* src, u32 width, u32 height) {
  for (u32 y = 0; y < height; y += 4) {
    for (u32 x = 0; x < width; x += 4) {
      for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
          rgba c = *(rgba*)&src[(y + row) * width + x + column];
          u16 packed = 0;
          if (c.a < 0xe0)
            packed = ((c.a & 0xe0) << 7) | ((c.r & 0xf0) << 4) | (c.g & 0xf0) |
                     ((c.b & 0xf0) >> 4);
          else
            packed = 0x8000 | ((c.r & 0xf8) << 7) | ((c.g & 0xf8) << 2) |
                     ((c.b & 0xf8) >> 3);
          dst[column * 2] = packed >> 8;
          dst[column * 2 + 1] = packed & 0xFF;
        }
        dst += 8;
      }
    }
  }
}

void encodeRGBA8(u8* dst, const u32* src4, u32 width, u32 height) {
#if 0
  for (int y = 0; y < height; y += 4) {
    for (int x = 0; x < width; x += 4) {

      for (int row = 0; row < 2; ++row) {
        for (int column = 0; column < 4; ++column) {
          rgba c = *(rgba*)&src[(y + row) * width + x + column];
          rgba c2 = *(rgba*)&src[(y + row) * width + x + column + 16];
          dst[column * 4] = c.a;
          dst[column * 4 + 1] = c2.g;
          dst[column * 4 + 2] = c2.b;
          dst[column * 4 + 3] = c.r;
        }
        dst += 64;
      }
    }
  }
#endif
  // From MP
  const u8* src = reinterpret_cast<const u8*>(src4);
  for (uint16_t block = 0; block < height; block += 4) {
    for (uint16_t i = 0; i < width; i += 4) {

      for (uint32_t c = 0; c < 4; c++) {
        uint32_t blockWid = (((block + c) * width) + i) << 2;

        *dst++ = src[blockWid + 3]; // ar = 0
        *dst++ = src[blockWid + 0];
        *dst++ = src[blockWid + 7]; // ar = 1
        *dst++ = src[blockWid + 4];
        *dst++ = src[blockWid + 11]; // ar = 2
        *dst++ = src[blockWid + 8];
        *dst++ = src[blockWid + 15]; // ar = 3
        *dst++ = src[blockWid + 12];
      }
      for (uint32_t c = 0; c < 4; c++) {
        uint32_t blockWid = (((block + c) * width) + i) << 2;

        *dst++ = src[blockWid + 1]; // gb = 0
        *dst++ = src[blockWid + 2];
        *dst++ = src[blockWid + 5]; // gb = 1
        *dst++ = src[blockWid + 6];
        *dst++ = src[blockWid + 9]; // gb = 2
        *dst++ = src[blockWid + 10];
        *dst++ = src[blockWid + 13]; // gb = 3
        *dst++ = src[blockWid + 14];
      }
    }
  }
}

} // namespace image
} // namespace librii
