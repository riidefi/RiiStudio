#ifndef RII_ENCODER_H
#define RII_ENCODER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t rii_compute_image_size(uint32_t format, uint32_t width,
                                uint32_t height);

uint32_t rii_compute_image_size_mip(uint32_t format, uint32_t width,
                                    uint32_t height, uint32_t number_of_images);

void rii_encode_cmpr(void* dst, size_t dst_len, const void* src, size_t src_len,
                     unsigned int width, unsigned int height);

void rii_encode_i4(void* dst, size_t dst_len, const void* src, size_t src_len,
                   unsigned int width, unsigned int height);

void rii_encode_i8(void* dst, size_t dst_len, const void* src, size_t src_len,
                   unsigned int width, unsigned int height);

void rii_encode_ia4(void* dst, size_t dst_len, const void* src, size_t src_len,
                    unsigned int width, unsigned int height);

void rii_encode_ia8(void* dst, size_t dst_len, const void* src, size_t src_len,
                    unsigned int width, unsigned int height);

void rii_encode_rgb565(void* dst, size_t dst_len, const void* src,
                       size_t src_len, unsigned int width, unsigned int height);

void rii_encode_rgb5a3(void* dst, size_t dst_len, const void* src,
                       size_t src_len, unsigned int width, unsigned int height);

void rii_encode_rgba8(void* dst, size_t dst_len, const void* src,
                      size_t src_len, unsigned int width, unsigned int height);

void rii_decode(void* dst, size_t dst_len, const void* src, size_t src_len,
                uint32_t width, uint32_t height, uint32_t texformat,
                const void* tlut, size_t tlut_len, uint32_t tlutformat);

int32_t gctex_get_version_unstable_api(char* buf, uint32_t len);

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
