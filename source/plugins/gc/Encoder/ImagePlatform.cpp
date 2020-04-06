#include "ImagePlatform.hpp"

#include <vendor/dolemu/TextureDecoder/TextureDecoder.h>
#include <vendor/ogc/texture.h>
#include <vendor/avir/avir.h>
#include <vendor/avir/lancir.h>
#include "CmprEncoder.hpp"
#include <vendor/mp/Metaphrasis.h>

namespace libcube::image_platform {


std::pair<int, int> getBlockedDimensions(int width, int height, gx::TextureFormat format) {
	assert(width > 0 && height > 0);
	const u32 blocksize = format == gx::TextureFormat::RGBA8 ? 64 : 32;

	return {
		(width + blocksize - 1) & ~(blocksize - 1),
		(height + blocksize - 1) & ~(blocksize - 1)
	};
}

int getEncodedSize(int width, int height, gx::TextureFormat format, u32 mipMapCount) {
	return GetTexBufferSize(width, height, static_cast<u32>(format), mipMapCount != 0, mipMapCount + 1);
}

// X -> raw 8-bit RGBA
void decode(u8* dst, const u8* src, int width, int height, gx::TextureFormat texformat,
	const u8* tlut, gx::PaletteFormat tlutformat) {
	return TexDecoder_Decode(dst, src, width, height, static_cast<TextureFormat>(texformat), tlut, static_cast<TLUTFormat>(tlutformat));
}

typedef std::unique_ptr<uint32_t[]> codec_t(uint32_t*, uint16_t, uint16_t);
std::array<void*, 8> codecs {
	&convertBufferToI4,
	&convertBufferToI8,
	&convertBufferToIA4,
	&convertBufferToIA8,
	&convertBufferToRGB565,
	&convertBufferToRGB5A3,
	&convertBufferToRGBA8
};
static std::unique_ptr<uint32_t[]> invokeCodec(uint32_t* rgbaBuf, uint16_t width, uint16_t height, gx::TextureFormat texformat) {
	int id = static_cast<int>(texformat);
	assert(id <= codecs.size());
	if (static_cast<int>(texformat) <= codecs.size()) {
		return reinterpret_cast<codec_t*>(codecs[id])(rgbaBuf, width, height);
	} else {
		return nullptr;
	}
}
// raw 8-bit RGBA -> X
void encode(u8* dst, const u8* src, int width, int height, gx::TextureFormat texformat) {
	if (texformat == gx::TextureFormat::CMPR) {
		EncodeDXT1(dst, src, width, height);
	} else if (static_cast<int>(texformat) <= static_cast<int>(gx::TextureFormat::RGBA8)) {
		// TODO: MP does not work on non block dims?
		assert(getBlockedDimensions(width, height, texformat) == std::make_pair<int, int>(width, height));

		// Until we replace this lib...
		uint32_t* rgbaBuf = const_cast<uint32_t*>(reinterpret_cast<const uint32_t*>(src));

		// TODO: MP allocates for us. We don't want this.
		auto newBuf = invokeCodec(rgbaBuf, width, height, texformat);

		// Copy the temp buffer..
		assert(newBuf.get());
		memcpy(dst, newBuf.get(), getEncodedSize(width, height, texformat, 0));
	} else {
		// No palette support
		assert(false);
	}
}
// Change format, no resizing
void reencode(u8* dst, const u8* src, int width, int height,
	gx::TextureFormat oldFormat, gx::TextureFormat newFormat) {
	std::vector<u8> tmp(width * height * 4);
	decode(tmp.data(), src, width, height, oldFormat);
	encode(dst, tmp.data(), width, height, newFormat);
}

avir::CImageResizer<> Avir8BitImageResizer(8);
avir::CLancIR AvirLanczos;
void resize(u8* dst, int dx, int dy, const u8* src, int sx, int sy, ResizingAlgorithm type) {
	bool dstSrcTmp = dst == src;
	u8* realDst = nullptr;
	std::vector<u8> tmp(0);

	assert(dst != nullptr);
	if (dst == nullptr) {
		return;
	} else if (src = nullptr) {
		src = dst;
	}

	if (dstSrcTmp) {
		tmp.resize(dx * dy * 4);
		realDst = dst;
		dst = tmp.data();
	}
	if (type == ResizingAlgorithm::AVIR) {
		// TODO: Allow more customization (args, k)
		Avir8BitImageResizer.resizeImage(src, sx, sy, 0, dst, dx, dy, 4, 0);
	}
	else {
		AvirLanczos.resizeImage(src, sx, sy, 0, dst, dx, dy, 4, 0);
	}

	if (dstSrcTmp) {
		assert(realDst);
		memcpy(realDst, dst, tmp.size());
	}
}

void transform(u8* dst, int dwidth, int dheight,
	gx::TextureFormat oldformat,
	std::optional<gx::TextureFormat> newformat,
	const u8* src, int swidth, int sheight,
	u32 mipMapCount,
	ResizingAlgorithm algorithm)
{
	assert(dst);
	assert(dwidth > 0 && dheight > 0);
	if (swidth <= 0) swidth = dwidth;
	if (sheight <= 0) sheight = dheight;
	if (src == nullptr) src = dst;
	if (!newformat.has_value()) newformat = oldformat;

	// Determine whether to decode this sublevel as an image or many sublvels.
	if (mipMapCount > 0) {
		for (u32 i = 0; i < mipMapCount; ++i) {
			const auto src_lod_ofs = getEncodedSize(swidth, sheight, oldformat, i);
			const auto dst_lod_ofs = getEncodedSize(dwidth, dheight, newformat.value(), i);
			const auto src_lod_x = swidth >> i;
			const auto src_lod_y = sheight >> i;
			const auto dst_lod_x = dwidth >> i;
			const auto dst_lod_y = dheight >> i;
			transform(dst + dst_lod_ofs, dst_lod_x, dst_lod_y, oldformat, newformat.value(),
				src + src_lod_ofs, src_lod_x, src_lod_y, 0, algorithm);
		}
	} else {
		// First decode the source image
		std::vector<u8> srcDecoded(0);
		const u8* pSrcDecoded = nullptr;
		const bool srcTmp = oldformat != gx::TextureFormat::Extension_RawRGBA32;
		if (srcTmp) {
			srcDecoded.resize(swidth * sheight * 4);
			decode(srcDecoded.data(), src, swidth, sheight, oldformat);
			pSrcDecoded = srcDecoded.data();
		} else {
			pSrcDecoded = src;
		}
		assert(pSrcDecoded);

		// If the dst format is raw and not the source buf, we can directly write to it.
		// Otherwise, we need to make a buffer.
		std::vector<u8> dstDecoded(0);
		u8* pDstDecoded = nullptr;
		const bool dstTmp = newformat.value() != gx::TextureFormat::Extension_RawRGBA32 ||
			dst == pSrcDecoded;
		if (dstTmp) {
			dstDecoded.resize(dwidth * dheight * 4);
			pDstDecoded = dstDecoded.data();
		} else {
			pDstDecoded = dst;
		}
		assert(pDstDecoded);

		// If the dimensions differ, resize
		if (swidth != dwidth || sheight != dheight) {
			resize(pDstDecoded, dwidth, dheight, pSrcDecoded, swidth, sheight, algorithm);
		}

		// Now we need to encode.
		if (newformat != gx::TextureFormat::Extension_RawRGBA32) {
			encode(dst, pDstDecoded, dwidth, dheight, newformat.value());
		} else if (dstTmp) { // The two buffers are equal
			memcpy(dst, pDstDecoded, dstDecoded.size());
		}
	}
}


}
