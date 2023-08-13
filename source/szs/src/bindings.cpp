#include "SZS.hpp"

// Prevent duplicate symbols from Rust and C++ side
namespace librii {
using namespace rlibrii;
}

extern "C" {
uint32_t impl_rii_is_szs_compressed(const void* src, uint32_t len) {
  (void)src;
  (void)len;
  return 0;
}
uint32_t impl_rii_get_szs_expand_size(const void* src, uint32_t len) {
  (void)src;
  (void)len;
  return 0;
}
uint32_t impl_rii_szs_decode(void* buf, uint32_t len, const void* src,
                             uint32_t src_len) {
  (void)buf;
  (void)len;
  (void)src;
  (void)src_len;
  return 0;
}
uint32_t impl_rii_worst_encoding_size(const void* src, uint32_t len) {
  (void)src;
  (void)len;
  return 0;
}

#define RMIN(x, y) (x < y ? x : y)
static inline char* my_strdup(const char* in) {
  auto len = strlen(in);
  char* buf = (char*)malloc(len + 1);
  memcpy(buf, in, len);
  buf[len] = '\0';
  return buf;
}
const char* impl_rii_encodeAlgo(void* dst, uint32_t dst_len, const void* src,
                                uint32_t src_len, uint32_t* used_len,
                                uint32_t algo) {
  std::span<const u8> src_span{(const u8*)src, src_len};
  if (algo > 3) {
    return my_strdup("Invalid algorithm");
  }
  auto algo_e = static_cast<librii::szs::Algo>(algo);
  auto res = librii::szs::encodeAlgo(src_span, algo_e);
  if (!res) {
    return my_strdup(res.error().c_str());
  }
  if (res->size() > dst_len) {
    return my_strdup("Destination buffer is too small");
  }
  memcpy(dst, res->data(), RMIN(res->size(), static_cast<size_t>(dst_len)));
  if (!used_len) {
    return my_strdup("used_len was NULL");
  }
  *used_len = RMIN(res->size(), static_cast<size_t>(dst_len));
  return nullptr;
}
}
