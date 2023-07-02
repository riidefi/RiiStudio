#ifndef RII_ENCODER_H
#define RII_ENCODER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* RII_ENCODER_H */
