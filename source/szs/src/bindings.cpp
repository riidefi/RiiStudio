#include "SZS.hpp"

extern "C" {
uint32_t impl_rii_is_szs_compressed(const void* src, uint32_t len) { return 0; }
uint32_t impl_rii_get_szs_expand_size(const void* src, uint32_t len) {
  return 0;
}
uint32_t impl_rii_szs_decode(void* buf, uint32_t len, const void* src,
                             uint32_t src_len) {
  return 0;
}
uint32_t impl_rii_worst_encoding_size(const void* src, uint32_t len) {
  return 0;
}
}
