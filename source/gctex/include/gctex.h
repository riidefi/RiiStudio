#ifndef RII_ENCODER_H
#define RII_ENCODER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//! Computes the size of a simple image
uint32_t rii_compute_image_size(uint32_t format, uint32_t width,
                                uint32_t height);

//! Computes the size of a mipmapped image
uint32_t rii_compute_image_size_mip(uint32_t format, uint32_t width,
                                    uint32_t height, uint32_t number_of_images);

//! Encode from raw 32-bit color to the specified format
void rii_encode(uint32_t format, void* dst, uint32_t dst_len, const void* src,
                uint32_t src_len,
                uint32_t width, uint32_t height);

//! Decode from the specified format to raw 32-bit color
void rii_decode(void* dst, uint32_t dst_len, const void* src, uint32_t src_len,
                uint32_t width, uint32_t height, uint32_t texformat,
                const void* tlut, uint32_t tlut_len, uint32_t tlutformat);

//! Returns the length of the string. Will be no more than 256.
int32_t gctex_get_version_unstable_api(char* buf, uint32_t len);

//
// Implementations
//

void rii_encode_cmpr(void* dst, uint32_t dst_len, const void* src,
                     uint32_t src_len, uint32_t width, uint32_t height);

void rii_encode_i4(void* dst, uint32_t dst_len, const void* src,
                   uint32_t src_len, uint32_t width, uint32_t height);

void rii_encode_i8(void* dst, uint32_t dst_len, const void* src,
                   uint32_t src_len, uint32_t width, uint32_t height);

void rii_encode_ia4(void* dst, uint32_t dst_len, const void* src,
                    uint32_t src_len, uint32_t width, uint32_t height);

void rii_encode_ia8(void* dst, uint32_t dst_len, const void* src,
                    uint32_t src_len, uint32_t width, uint32_t height);

void rii_encode_rgb565(void* dst, uint32_t dst_len, const void* src,
                       uint32_t src_len, uint32_t width, uint32_t height);

void rii_encode_rgb5a3(void* dst, uint32_t dst_len, const void* src,
                       uint32_t src_len, uint32_t width, uint32_t height);

void rii_encode_rgba8(void* dst, uint32_t dst_len, const void* src,
                      uint32_t src_len, uint32_t width, uint32_t height);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <string>

namespace gctex {

static inline std::string get_version() {
  std::string s;
  s.resize(1024);
  int32_t len = gctex_get_version_unstable_api(s.data(), s.size());
  if (len <= 0 || len > s.size()) {
    return "Unable to query";
  }
  return s;
}

} // namespace gctex
#endif

#endif /* RII_ENCODER_H */
