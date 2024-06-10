#pragma once

#include <stdint.h>

#define SZS_EXPORT extern

#ifdef __cplusplus
extern "C" {
#endif

SZS_EXPORT const char* impl_riiszs_decode(void* buf, uint32_t len,
                                          const void* src, uint32_t src_len);
SZS_EXPORT uint32_t impl_rii_worst_encoding_size(uint32_t len);
SZS_EXPORT const char* impl_rii_encodeAlgo(void* dst, uint32_t dst_len,
                                           const void* src, uint32_t src_len,
                                           uint32_t* used_len, uint32_t algo);

#ifdef __cplusplus
} // extern "C"
#endif
