#pragma once

#include <core/common.h>

#include <tuple>
#include <optional>

#include <plugins/gc/GX/Material.hpp>

namespace libcube::image_platform {


std::pair<int, int> getBlockedDimensions(int width, int height, gx::TextureFormat format);
int getEncodedSize(int width, int height, gx::TextureFormat format, u32 mipMapCount= 0); // 0 - just base image

// For efficiency, encode/decode will not handle dst==src case for you!
// X -> raw 8-bit RGBA
void decode(u8* dst, const u8* src, int width, int height, gx::TextureFormat texformat,
	const u8* tlut=nullptr, gx::PaletteFormat tlutformat=gx::PaletteFormat::IA8);
// raw 8-bit RGBA -> X
void encode(u8* dst, const u8* src, int width, int height, gx::TextureFormat texformat);

enum ResizingAlgorithm {
	AVIR,
	Lanczos
};

// dst and source may be equal
// raw 8-bit RGBA resize
void resize(u8* dst, int dx, int dy, const u8* src, int sx, int sy, ResizingAlgorithm type = ResizingAlgorithm::AVIR);

// dst and source may be equal
// extension gx::TextureFormat::Extension_RawRGBA32 is supported
// if srctype is not set, it is implied to be dsttype
// if src is nullptr, it is dst
// if srcwidth/srcheight are zero or below, they are dst dimensions
// If mipMapCount is 0, no sublevels. 1 means one sublevel.
void transform(u8* dst, int dwidth, int dheight,
	gx::TextureFormat oldformat = gx::TextureFormat::Extension_RawRGBA32,
	std::optional<gx::TextureFormat> newformat = std::nullopt,
	const u8* src=nullptr, int swidth=-1, int sheight=-1,
	u32 mipMapCount = 0,
	ResizingAlgorithm algorithm = ResizingAlgorithm::AVIR);


} // namespace libcube::image_platform
