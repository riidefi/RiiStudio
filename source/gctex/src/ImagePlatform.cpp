#include <string.h>

#include "dolemu/TextureDecoder/TextureDecoder.h"

namespace librii {
namespace image {

// X -> raw 8-bit RGBA
void decode(u8* dst, const u8* src, u32 width, u32 height, u32 texformat,
            const u8* tlut, u32 tlutformat) {
  TexDecoder_Decode(dst, src, static_cast<int>(width), static_cast<int>(height),
                    static_cast<TextureFormat>(texformat), tlut,
                    static_cast<TLUTFormat>(tlutformat));
}

} // namespace image
} // namespace librii
