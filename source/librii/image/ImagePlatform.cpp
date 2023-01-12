#include "ImagePlatform.hpp"

#include "CmprEncoder.hpp"
#include <librii/gx.h>
#include <vendor/avir/avir.h>
#include <vendor/avir/lancir.h>
#include <vendor/dolemu/TextureDecoder/TextureDecoder.h>

#include <rsl/Ranges.hpp>

IMPORT_STD;

namespace librii::image {

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

// X -> raw 8-bit RGBA
void decode(u8* dst, const u8* src, int width, int height,
            gx::TextureFormat texformat, const u8* tlut,
            gx::PaletteFormat tlutformat) {
  TexDecoder_Decode(dst, src, width, height,
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
  return static_cast<float>(rgba.r) * 0.299 +
         static_cast<float>(rgba.g) * 0.587 +
         static_cast<float>(rgba.b) * 0.144;
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
          dst[column * 2] = luminosity(c) << 8;
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

// raw 8-bit RGBA -> X
Result<void> encode(u8* dst, const u8* src, int width, int height,
                    gx::TextureFormat texformat) {
  if (texformat == gx::TextureFormat::CMPR) {
    EncodeDXT1(dst, src, width, height);
    return {};
  }

  if (texformat == gx::TextureFormat::I4) {
    encodeI4(dst, (const u32*)src, width, height);
    return {};
  }

  if (texformat == gx::TextureFormat::I8) {
    encodeI8(dst, (const u32*)src, width, height);
    return {};
  }

  if (texformat == gx::TextureFormat::IA4) {
    encodeIA4(dst, (const u32*)src, width, height);
    return {};
  }

  if (texformat == gx::TextureFormat::IA8) {
    encodeIA8(dst, (const u32*)src, width, height);
    return {};
  }

  if (texformat == gx::TextureFormat::RGB565) {
    encodeRGB565(dst, (const u32*)src, width, height);
    return {};
  }

  if (texformat == gx::TextureFormat::RGB5A3) {
    encodeRGB5A3(dst, (const u32*)src, width, height);
    return {};
  }

  if (texformat == gx::TextureFormat::RGBA8) {
    encodeRGBA8(dst, (const u32*)src, width, height);
    return {};
  }

  // No palette support
  return std::unexpected("No palette support");
}
// Change format, no resizing
Result<void> reencode(u8* dst, const u8* src, int width, int height,
                      gx::TextureFormat oldFormat,
                      gx::TextureFormat newFormat) {
  std::vector<u8> tmp(width * height * 4 + 1024 /* Bias for SIMD */);
  decode(tmp.data(), src, width, height, oldFormat);
  return encode(dst, tmp.data(), width, height, newFormat);
}

void resize(std::span<u8> dst, int dx, int dy, std::span<const u8> src, int sx,
            int sy, ResizingAlgorithm type) {
  std::vector<u8> src_(src.begin(), src.end());
  std::vector<u8> dst_(dst.begin(), dst.end());
  if (type == ResizingAlgorithm::AVIR) {
    avir::CImageResizer<> Avir8BitImageResizer(8);
    // TODO: Allow more customization (args, k)
    Avir8BitImageResizer.resizeImage(src_.data(), sx, sy, 0, dst_.data(), dx,
                                     dy, 4, 0);
  } else {
    avir::CLancIR AvirLanczos;
    AvirLanczos.resizeImage(src_.data(), sx, sy, 0, dst_.data(), dx, dy, 4, 0);
  }

  for (size_t i = 0; i < dst_.size(); ++i) {
    dst[i] = dst_[i];
  }
}

struct RGBA32ImageSource {
  static Result<RGBA32ImageSource> make(std::span<const u8> buf, int w, int h,
                                        gx::TextureFormat fmt) {
    RGBA32ImageSource tmp;
    tmp.mBuf = buf | rsl::ToList();
    tmp.mW = w;
    tmp.mH = h;
    tmp.mFmt = fmt;
    if (tmp.mFmt == gx::TextureFormat::Extension_RawRGBA32) {
      tmp.mDecoded = buf | rsl::ToList();
    } else {
      tmp.mTmp.resize(roundUp(w, 32) * roundUp(h, 32) *
                      4 /* Round up for SIMD */);
      EXPECT(tmp.mBuf.size() >= getEncodedSize(w, h, tmp.mFmt));
      decode(tmp.mTmp.data(), tmp.mBuf.data(), w, h, tmp.mFmt);
      tmp.mDecoded = tmp.mTmp;
    }
    return tmp;
  }

  std::span<const u8> get() const { return mDecoded; }
  u32 size() const { return mW * mH * 4; }
  int width() const { return mW; }
  int height() const { return mH; }

private:
  std::vector<u8> mTmp = {};
  std::vector<u8> mDecoded = {};
  std::vector<u8> mBuf = {};
  int mW = {};
  int mH = {};
  gx::TextureFormat mFmt = {gx::TextureFormat::Extension_RawRGBA32};
};
struct RGBA32ImageTarget {
  RGBA32ImageTarget(int w, int h) : mW(w), mH(h) {
    mTmp.resize(roundUp(w, 32) * roundUp(h, 32) * 4);
  }
  [[nodiscard]] Result<void> copyTo(std::span<u8> dst, gx::TextureFormat fmt) {
    if (fmt == gx::TextureFormat::Extension_RawRGBA32) {
      EXPECT(dst.size() >= mTmp.size());
      memcpy(dst.data(), mTmp.data(), mTmp.size());
    } else {
      EXPECT(dst.size() >= getEncodedSize(mW, mH, fmt));
      EXPECT(mTmp.size() >= mW * mH * 4);
      return encode(dst.data(), mTmp.data(), mW, mH, fmt);
    }
    return {};
  }
  void fromOtherSized(std::span<const u8> src, u32 ow, u32 oh,
                      ResizingAlgorithm al) {
    mTmp.resize(mW * mH * 4);
    // assert(src.size() >= mTmp.size());
    if (ow == mW && oh == mH) {
      memcpy(mTmp.data(), src.data(), mTmp.size());
    } else {
      resize(mTmp, mW, mH, src, ow, oh, al);
    }
  }
  void fromOtherSized(const RGBA32ImageSource& source, ResizingAlgorithm al) {
    fromOtherSized(source.get(), source.width(), source.height(), al);
  }
  std::span<u8> get() { return {mTmp.data(), mTmp.size()}; }
  std::span<const u8> get() const { return {mTmp.data(), mTmp.size()}; }
  u32 size() const { return mW * mH * 4; }

private:
  std::vector<u8> mTmp;
  int mW;
  int mH;
};
[[nodiscard]] Result<void> transform(std::span<u8> dst_, int dwidth,
                                     int dheight, gx::TextureFormat oldformat,
                                     std::optional<gx::TextureFormat> newformat,
                                     std::span<const u8> src_, int swidth,
                                     int sheight, u32 mipMapCount,
                                     ResizingAlgorithm algorithm) {
  std::vector<u8> dst(dst_.begin(), dst_.end());
  std::vector<u8> src(src_.begin(), src_.end());
#ifdef IMAGE_DEBUG
  printf(
      "Transform: Dest={%p, w:%i, h:%i}, Source={%p, w:%i, h:%i}, NumMip=%u\n",
      dst.data(), dwidth, dheight, src.data(), swidth, sheight, mipMapCount);
#endif // IMAGE_DEBUG
  EXPECT(!dst.empty());
  EXPECT(!src.empty());
  EXPECT(dwidth > 0 && dheight > 0);
  if (swidth <= 0)
    swidth = dwidth;
  if (sheight <= 0)
    sheight = dheight;
  if (src.empty())
    src = dst;
  if (!newformat.has_value())
    newformat = oldformat;

  // Determine whether to decode this sublevel as an image or many sublvels.
  if (mipMapCount >= 1) {
    if (!is_power_of_2(swidth) || !is_power_of_2(sheight) ||
        !is_power_of_2(dwidth) || !is_power_of_2(dheight)) {
      return std::unexpected(
          "It is a GPU hardware requirement that mipmaps be powers of two");
    }
    TRY(transform(dst, dwidth, dheight, oldformat, newformat, src, swidth,
                  sheight, 0, algorithm));
    for (u32 i = 1; i <= mipMapCount; ++i) {
      const auto src_lod_ofs =
          getEncodedSize(swidth, sheight, oldformat, i - 1);
      const auto dst_lod_ofs =
          getEncodedSize(dwidth, dheight, newformat.value(), i - 1);
      const auto src_lod_x = swidth >> i;
      const auto src_lod_y = sheight >> i;
      auto dst_lod_x = dwidth >> i;
      auto dst_lod_y = dheight >> i;

      TRY(transform(std::span(dst).subspan(dst_lod_ofs), dst_lod_x, dst_lod_y,
                    oldformat, newformat.value(),
                    std::span(src).subspan(src_lod_ofs), src_lod_x, src_lod_y,
                    0, algorithm));
    }
  } else {
    EXPECT(swidth > 0 && sheight > 0);
    auto source = TRY(RGBA32ImageSource::make(src, swidth, sheight, oldformat));
    EXPECT(source.get().size());

    // TODO: We don't always need to allocate this
    RGBA32ImageTarget target(dwidth, dheight);
    EXPECT(target.get().size());

    target.fromOtherSized(source, algorithm);

    // TODO: A copy here can be prevented
    TRY(target.copyTo(dst, newformat.value()));
  }

  EXPECT(dst_.size() == dst.size());
  for (size_t i = 0; i < dst_.size(); ++i) {
    dst_[i] = dst[i];
  }
  return {};
}

} // namespace librii::image
