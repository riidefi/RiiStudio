#include "ImagePlatform.hpp"

#include "CmprEncoder.hpp"
#include <vendor/avir/avir.h>
#include <vendor/avir/lancir.h>
#include <vendor/dolemu/TextureDecoder/TextureDecoder.h>
#include <vendor/mp/Metaphrasis.h>
#include <vendor/ogc/texture.h>

namespace libcube::image_platform {

std::pair<int, int> getBlockedDimensions(int width, int height,
                                         gx::TextureFormat format) {
  assert(width > 0 && height > 0);
  const u32 blocksize = format == gx::TextureFormat::RGBA8 ? 64 : 32;

  return {(width + blocksize - 1) & ~(blocksize - 1),
          (height + blocksize - 1) & ~(blocksize - 1)};
}

int getEncodedSize(int width, int height, gx::TextureFormat format,
                   u32 mipMapCount) {
  if (format == libcube::gx::TextureFormat::Extension_RawRGBA32)
    return width * height * 4;
  return GetTexBufferSize(width, height, static_cast<u32>(format),
                          mipMapCount != 0, mipMapCount + 1);
}

// X -> raw 8-bit RGBA
void decode(u8 *dst, const u8 *src, int width, int height,
            gx::TextureFormat texformat, const u8 *tlut,
            gx::PaletteFormat tlutformat) {
  return TexDecoder_Decode(dst, src, width, height,
                           static_cast<TextureFormat>(texformat), tlut,
                           static_cast<TLUTFormat>(tlutformat));
}

typedef std::unique_ptr<uint32_t[]> codec_t(uint32_t *, uint16_t, uint16_t);
codec_t *codecs[8]{convertBufferToI4,     convertBufferToI8,
                   convertBufferToIA4,    convertBufferToIA8,
                   convertBufferToRGB565, convertBufferToRGB5A3,
                   convertBufferToRGBA8};
static std::unique_ptr<uint32_t[]> invokeCodec(uint32_t *rgbaBuf,
                                               uint16_t width, uint16_t height,
                                               gx::TextureFormat texformat) {
  int id = static_cast<int>(texformat);
  assert(id <= 8);
  if (static_cast<int>(texformat) <= 8) {
    return codecs[id](rgbaBuf, width, height);
  } else {
    return nullptr;
  }
}
// raw 8-bit RGBA -> X
void encode(u8 *dst, const u8 *src, int width, int height,
            gx::TextureFormat texformat) {
  if (texformat == gx::TextureFormat::CMPR) {
    EncodeDXT1(dst, src, width, height);
  } else if (static_cast<int>(texformat) <=
             static_cast<int>(gx::TextureFormat::RGBA8)) {
    // TODO: MP does not work on non block dims?
    const auto inpair = std::pair<int, int>(width, height);
    assert(getBlockedDimensions(width, height, texformat) == inpair);

    // Until we replace this lib...
    uint32_t *rgbaBuf =
        const_cast<uint32_t *>(reinterpret_cast<const uint32_t *>(src));

    // TODO: MP allocates for us. We don't want this.
    auto newBuf = invokeCodec(rgbaBuf, width, height, texformat);

    // Copy the temp buffer..
    assert(newBuf.get());
    const auto encoded_size = getEncodedSize(width, height, texformat, 0);
    memcpy(dst, newBuf.get(), encoded_size);
  } else {
    // No palette support
    assert(false);
  }
}
// Change format, no resizing
void reencode(u8 *dst, const u8 *src, int width, int height,
              gx::TextureFormat oldFormat, gx::TextureFormat newFormat) {
  std::vector<u8> tmp(width * height * 4);
  decode(tmp.data(), src, width, height, oldFormat);
  encode(dst, tmp.data(), width, height, newFormat);
}

void resize(u8 *dst, int dx, int dy, const u8 *src, int sx, int sy,
            ResizingAlgorithm type) {
  bool dstSrcTmp = dst == src;
  u8 *realDst = nullptr;
  std::vector<u8> tmp(0);

  assert(dst != nullptr);
  if (dst == nullptr) {
    return;
  } else if (src == nullptr) {
    src = dst;
  }

  if (dstSrcTmp) {
    tmp.resize(dx * dy * 4);
    realDst = dst;
    dst = tmp.data();
  }
  if (type == ResizingAlgorithm::AVIR) {

    avir::CImageResizer<> Avir8BitImageResizer(8);
    // TODO: Allow more customization (args, k)
    Avir8BitImageResizer.resizeImage(src, sx, sy, 0, dst, dx, dy, 4, 0);
    assert(memcmp(src, dst, sx * sy * 4));
  } else {
    avir::CLancIR AvirLanczos;
    AvirLanczos.resizeImage(src, sx, sy, 0, dst, dx, dy, 4, 0);
  }

  if (dstSrcTmp) {
    assert(realDst);
    memcpy(realDst, dst, tmp.size());
  }
}

struct RGBA32ImageSource {
  RGBA32ImageSource(const u8 *buf, int w, int h, gx::TextureFormat fmt)
      : mBuf(buf), mW(w), mH(h), mFmt(fmt) {
    if (fmt == gx::TextureFormat::Extension_RawRGBA32) {
      mDecoded = buf;
    } else {
      mTmp.resize(w * h * 4);
      decode(mTmp.data(), mBuf, w, h, fmt);
      mDecoded = mTmp.data();
    }
  }

  const u8 *get() const { return mDecoded; }
  u32 size() const { return mW * mH * 4; }
  int width() const { return mW; }
  int height() const { return mH; }

private:
  std::vector<u8> mTmp;
  const u8 *mDecoded = nullptr;
  const u8 *mBuf;
  int mW;
  int mH;
  gx::TextureFormat mFmt;
};
struct RGBA32ImageTarget {
  RGBA32ImageTarget(int w, int h) : mW(w), mH(h) { mTmp.resize(w * h * 4); }
  void copyTo(u8 *dst, gx::TextureFormat fmt) {
    if (fmt == gx::TextureFormat::Extension_RawRGBA32) {
      memcpy(dst, mTmp.data(), mTmp.size());
    } else {
      encode(dst, mTmp.data(), mW, mH, fmt);
    }
  }
  void fromOtherSized(const u8 *src, u32 ow, u32 oh, ResizingAlgorithm al) {
    if (ow == mW && oh == mH) {
      memcpy(mTmp.data(), src, mTmp.size());
    } else {
      resize(mTmp.data(), mW, mH, src, ow, oh, al);
    }
  }
  void fromOtherSized(RGBA32ImageSource &source, ResizingAlgorithm al) {
    fromOtherSized(source.get(), source.width(), source.height(), al);
  }
  u8 *get() { return mTmp.data(); }
  u32 size() const { return mW * mH * 4; }

private:
  std::vector<u8> mTmp;
  int mW;
  int mH;
};
void transform(u8 *dst, int dwidth, int dheight, gx::TextureFormat oldformat,
               std::optional<gx::TextureFormat> newformat, const u8 *src,
               int swidth, int sheight, u32 mipMapCount,
               ResizingAlgorithm algorithm) {
  assert(dst);
  assert(dwidth > 0 && dheight > 0);
  if (swidth <= 0)
    swidth = dwidth;
  if (sheight <= 0)
    sheight = dheight;
  if (src == nullptr)
    src = dst;
  if (!newformat.has_value())
    newformat = oldformat;

  // Determine whether to decode this sublevel as an image or many sublvels.
  if (mipMapCount > 0) {
    std::vector<u8> srcBuf(0);
    const u8 *pSrc = nullptr;
    if (dst == src) {
      srcBuf.resize(getEncodedSize(swidth, sheight, oldformat, mipMapCount));
      memcpy(srcBuf.data(), src, srcBuf.size());
      pSrc = srcBuf.data();
    } else {
      pSrc = src;
    }
    assert(pSrc);
    transform(dst, dwidth, dheight, oldformat, newformat, pSrc, swidth, sheight,
              0, algorithm);
    for (u32 i = 0; i < mipMapCount; ++i) {
      const auto src_lod_ofs = getEncodedSize(swidth, sheight, oldformat, i);
      const auto dst_lod_ofs =
          getEncodedSize(dwidth, dheight, newformat.value(), i);
      const auto src_lod_x = swidth >> i;
      const auto src_lod_y = sheight >> i;
      const auto dst_lod_x = dwidth >> i;
      const auto dst_lod_y = dheight >> i;
      transform(dst + dst_lod_ofs, dst_lod_x, dst_lod_y, oldformat,
                newformat.value(), pSrc + src_lod_ofs, src_lod_x, src_lod_y, 0,
                algorithm);
    }
  } else {
    RGBA32ImageSource source(src, swidth, sheight, oldformat);
    assert(source.get());

    // TODO: We don't always need to allocate this
    RGBA32ImageTarget target(dwidth, dheight);
    assert(target.get());

    target.fromOtherSized(source, algorithm);

    // TODO: A copy here can be prevented
    target.copyTo(dst, newformat.value());
  }
}

} // namespace libcube::image_platform
