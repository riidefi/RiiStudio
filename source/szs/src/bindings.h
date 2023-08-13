#include "util.h"

uint32_t impl_rii_is_szs_compressed(const void* src, uint32_t len);
uint32_t impl_rii_get_szs_expand_size(const void* src, uint32_t len);
uint32_t impl_rii_szs_decode(void* buf, uint32_t len, const void* src,
                        uint32_t src_len);
uint32_t impl_rii_worst_encoding_size(const void* src, uint32_t len);
