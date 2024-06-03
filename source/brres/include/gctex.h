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
                uint32_t src_len, uint32_t width, uint32_t height);

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

#include <span>
#include <string>
#include <vector>

namespace gctex {

//! Get the library version.
static inline std::string get_version() {
  std::string s;
  s.resize(1024);
  int32_t len = gctex_get_version_unstable_api(&s[0], s.size());
  if (len <= 0 || len > s.size()) {
    return "Unable to query";
  }
  s.resize(len);
  return s;
}

//! Computes the size of a simple image
static inline uint32_t compute_image_size(uint32_t format, uint32_t width,
                                          uint32_t height) {
  return ::rii_compute_image_size(format, width, height);
}

//! Computes the size of a mipmapped image
static inline uint32_t compute_image_size_mip(uint32_t format, uint32_t width,
                                              uint32_t height,
                                              uint32_t number_of_images) {
  return ::rii_compute_image_size_mip(format, width, height, number_of_images);
}

#ifndef GCTEX_DO_NOT_DEFINE_SPAN_APIS

//! Encode from raw 32-bit color to the specified format
static inline void encode_into(uint32_t format, std::span<uint8_t> dst,
                               std::span<const uint8_t> src, uint32_t width,
                               uint32_t height) {
  ::rii_encode(format, dst.data(), dst.size(), src.data(), src.size(), width,
               height);
}

//! Encode from raw 32-bit color to the specified format
static inline std::vector<uint8_t> encode(uint32_t format,
                                          std::span<const uint8_t> src,
                                          uint32_t width, uint32_t height) {
  std::vector<uint8_t> dst(::rii_compute_image_size(format, width, height));
  ::rii_encode(format, dst.data(), dst.size(), src.data(), src.size(), width,
               height);
  return dst;
}

//! Decode from the specified format to raw 32-bit color
static inline void decode_into(std::span<uint8_t> dst,
                               std::span<const uint8_t> src, uint32_t width,
                               uint32_t height, uint32_t texformat,
                               std::span<const uint8_t> tlut,
                               uint32_t tlutformat) {
  ::rii_decode(dst.data(), dst.size(), src.data(), src.size(), width, height,
               texformat, tlut.data(), tlut.size(), tlutformat);
}

//! Decode from the specified format to raw 32-bit color
static inline std::vector<uint8_t>
decode(std::span<const uint8_t> src, uint32_t width, uint32_t height,
       uint32_t texformat, std::span<const uint8_t> tlut, uint32_t tlutformat) {
  std::vector<uint8_t> dst(width * height * 4);
  ::rii_decode(dst.data(), dst.size(), src.data(), src.size(), width, height,
               texformat, tlut.data(), tlut.size(), tlutformat);
  return dst;
}

#endif

} // namespace gctex
#endif

#endif /* RII_ENCODER_H */
