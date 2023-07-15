#include "ImagePlatform.hpp"

#include <librii/gx.h>

#include <rsl/Ranges.hpp>

#include "avir-rs/include/avir_rs.h"
#include "gctex/include/gctex.h"

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
  rii_decode(dst, 0, src, 0, width, height, static_cast<u32>(texformat), tlut,
             0, static_cast<u32>(tlutformat));
}

// raw 8-bit RGBA -> X
Result<void> encode(u8* dst, const u8* src, int width, int height,
                    gx::TextureFormat texformat) {
  if (texformat == gx::TextureFormat::CMPR) {
    rii_encode_cmpr(dst, 0, src, 0, width, height);
    return {};
  }

  if (texformat == gx::TextureFormat::I4) {
    rii_encode_i4(dst, 0, src, 0, width, height);
    return {};
  }

  if (texformat == gx::TextureFormat::I8) {
    rii_encode_i8(dst, 0, src, 0, width, height);
    return {};
  }

  if (texformat == gx::TextureFormat::IA4) {
    rii_encode_ia4(dst, 0, src, 0, width, height);
    return {};
  }

  if (texformat == gx::TextureFormat::IA8) {
    rii_encode_ia8(dst, 0, src, 0, width, height);
    return {};
  }

  if (texformat == gx::TextureFormat::RGB565) {
    rii_encode_rgb565(dst, 0, src, 0, width, height);
    return {};
  }

  if (texformat == gx::TextureFormat::RGB5A3) {
    rii_encode_rgb5a3(dst, 0, src, 0, width, height);
    return {};
  }

  if (texformat == gx::TextureFormat::RGBA8) {
    rii_encode_rgba8(dst, 0, src, 0, width, height);
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
    avir_resize(dst_.data(), dst_.size(), dx, dy, src_.data(), src_.size(), sx,
                sy);
  } else {
    clancir_resize(dst_.data(), dst_.size(), dx, dy, src_.data(), src_.size(),
                   sx, sy);
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
  memcpy(&dst_[0], &dst[0], dst_.size());
  return {};
}

} // namespace librii::image
