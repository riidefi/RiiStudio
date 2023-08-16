#include "util.h"

#ifdef __EMSCRIPTEN__
#define WASM_EXPORT __attribute__((visibility("default")))
#else
#define WASM_EXPORT
#endif

WASM_EXPORT void impl_rii_decode(u8* dst, const u8* src, u32 width, u32 height,
                                 u32 texformat, const u8* tlut, u32 tlutformat);
WASM_EXPORT void impl_rii_encodeCMPR(u8* dest_img, const u8* source_img,
                                     u32 width, u32 height);
WASM_EXPORT void impl_rii_encodeI4(u8* dst, const u32* src, u32 width,
                                   u32 height);
WASM_EXPORT void impl_rii_encodeI8(u8* dst, const u32* src, u32 width,
                                   u32 height);
WASM_EXPORT void impl_rii_encodeIA4(u8* dst, const u32* src, u32 width,
                                    u32 height);
WASM_EXPORT void impl_rii_encodeIA8(u8* dst, const u32* src, u32 width,
                                    u32 height);
WASM_EXPORT void impl_rii_encodeRGB565(u8* dst, const u32* src, u32 width,
                                       u32 height);
WASM_EXPORT void impl_rii_encodeRGB5A3(u8* dst, const u32* src, u32 width,
                                       u32 height);
WASM_EXPORT void impl_rii_encodeRGBA8(u8* dst, const u32* src4, u32 width,
                                      u32 height);
