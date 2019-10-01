#include "TextureDecoding.hpp"
#include "ogc/texture.h"
#include "dolemu/TextureDecoder/TextureDecoder.h"

namespace TextureDecoding {

std::unique_ptr<u8> decode(const u8* source,
	int width, int height,
	u32 texture_format,
	u32 mipmap_count,
	const u8* palette, u32 palette_format)
{
	const u32 tex_size = GetTexBufferSize(width, height, texture_format, mipmap_count > 1, mipmap_count + 1);

	std::unique_ptr<u8> dest = std::unique_ptr<u8>(new u8[tex_size]);

	TexDecoder_Decode(dest.get(), source, width, height, (TextureFormat)texture_format, palette, (TLUTFormat)palette_format);

	return dest;
}

std::vector<u8> decodeVec(const u8 * source,
	int width, int height,
	u32 texture_format,
	u32 mipmap_count,
	const u8 * palette, u32 palette_format)
{
	const u32 tex_size = GetTexBufferSize(width, height, texture_format, mipmap_count > 1, mipmap_count + 1);

	std::vector<u8> dest(tex_size);

	TexDecoder_Decode(dest.data(), source, width, height, (TextureFormat)texture_format, palette, (TLUTFormat)palette_format);

	return dest;
}

} // namespace TextureDecoding
