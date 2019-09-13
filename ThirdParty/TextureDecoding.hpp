#pragma once

#include <oishii/types.hxx>
#include <memory>

namespace TextureDecoding {

std::unique_ptr<u8> decode(const u8* source,
	int width, int height,
	u32 texture_format,
	u32 mipmap_count,
	const u8* palette, u32 palette_format);


} // namespace TextureDecoding
