#include "SZS.hpp"
#include "SZSToSZP.hpp"
#include <string.h>

// Prevent duplicate symbols from Rust and C++ side
namespace librii {
using namespace rlibrii;
}

#define RMIN(x, y) (x < y ? x : y)
static inline char* my_strdup(const char* in) {
  auto len = strlen(in);
  char* buf = (char*)malloc(len + 1);
  memcpy(buf, in, len);
  buf[len] = '\0';
  return buf;
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
const char* impl_riiszs_decode(void* buf, uint32_t len, const void* src,
                               uint32_t src_len) {

  std::span<const u8> src_span{(const u8*)src, src_len};
  std::span<u8> dst_span{(u8*)buf, len};
  auto ok = librii::szs::decode(dst_span, src_span);
  if (!ok) {
    return my_strdup(ok.error().c_str());
  }
  return nullptr;
}
uint32_t impl_rii_worst_encoding_size(uint32_t len) {
  return librii::szs::getWorstEncodingSize(len);
}

const char* impl_rii_encodeAlgo(void* dst, uint32_t dst_len, const void* src,
                                uint32_t src_len, uint32_t* used_len,
                                uint32_t algo) {
  std::span<const u8> src_span{(const u8*)src, src_len};
  if (algo > 7) {
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

const char* impl_rii_deinterlace(void* dst, uint32_t dst_len, const void* src,
                                 uint32_t src_len, uint32_t* used_len) {
  if (!used_len) {
    return my_strdup("used_len was NULL");
  }
  std::span<u8> s_dst((u8*)dst, dst_len);
  std::span<const u8> s_src((const u8*)src, src_len);
  auto size = SZSToSZP_C(s_dst, s_src);
  if (!size) {
    return my_strdup(size.error().c_str());
  }
  *used_len = *size;
  return nullptr;
}
}
