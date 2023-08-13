#ifndef RII_SZS_H
#define RII_SZS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t rii_is_szs_compressed(const void* src, uint32_t len);
uint32_t rii_get_szs_expand_size(const void* src, uint32_t len);
uint32_t rii_szs_decode(void* buf, uint32_t len, const void* src,
                        uint32_t src_len);
uint32_t rii_worst_encoding_size(const void* src, uint32_t len);

enum {
  RII_SZS_ENCODE_ALGO_WORST_CASE_ENCODING,
  RII_SZS_ENCODE_ALGO_NINTENDO,
  RII_SZS_ENCODE_ALGO_MKWSP,
  RII_SZS_ENCODE_ALGO_CTGP,
};

#ifdef __cplusplus
}
#endif

#endif /* RII_SZS_H */
