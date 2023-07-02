#pragma once

#include "util.h"

namespace librii { namespace image {
void decode(u8* dst, const u8* src, u32 width, u32 height, u32 texformat,
            const u8* tlut, u32 tlutformat);
void encodeI4(u8* dst, const u32* src, u32 width, u32 height);
void encodeI8(u8* dst, const u32* src, u32 width, u32 height);
void encodeIA4(u8* dst, const u32* src, u32 width, u32 height);
void encodeIA8(u8* dst, const u32* src, u32 width, u32 height);
void encodeRGB565(u8* dst, const u32* src, u32 width, u32 height);
void encodeRGB5A3(u8* dst, const u32* src, u32 width, u32 height);
void encodeRGBA8(u8* dst, const u32* src4, u32 width, u32 height);
} } // namespace librii::images
