#pragma once

#include <memory>
#include <vector>
#include <stdint.h>
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;
typedef double f64;
namespace TextureDecoding {

std::unique_ptr<u8> decode(const u8* source,
	int width, int height,
	u32 texture_format,
	u32 mipmap_count,
	const u8* palette, u32 palette_format);

std::vector<u8> decodeVec(const u8* source,
	int width, int height,
	u32 texture_format,
	u32 mipmap_count,
	const u8* palette, u32 palette_format);

} // namespace TextureDecoding
