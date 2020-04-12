#pragma once

#include <core/common.h>

namespace libcube {

//! @brief Encode a RGBA32 buffer to GC DXT1.
//!
//! @param[in] dest   Pointer to the output buffer. Must be appropriately sized.
//! (Call procedure)
//! @param[in] source Pointer to the source buffer. Must be appropriately sized.
//! (width * height * 4)
//! @param[in] width  Width of the image.
//! @param[in] height Height of the image.
//!
void EncodeDXT1(u8* dest, const u8* source, u32 width, u32 height);

} // namespace libcube
