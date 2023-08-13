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

const char* riiszs_encode_algo_fast(void* dst, uint32_t dst_len,
                                    const void* src, uint32_t src_len,
                                    uint32_t* used_len, uint32_t algo);
void riiszs_free_error_message(const char* msg);

int32_t szs_get_version_unstable_api(char* buf, uint32_t len);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <expected>
#include <span>
#include <string>

namespace szs {

static inline std::string get_version() {
  std::string s;
  s.resize(1024);
  int32_t len = szs_get_version_unstable_api(s.data(), s.size());
  if (len <= 0 || len > s.size()) {
    return "Unable to query";
  }
  return s;
}

static inline std::expected<uint32_t, std::string>
encode_algo_fast(std::span<uint8_t> dst, std::span<const uint8_t> src,
                 uint32_t algo) {
  uint32_t used_len = 0;
  const char* err = riiszs_encode_algo_fast(dst.data(), dst.size(), src.data(),
                                            src.size(), &used_len, algo);
  if (err == nullptr) {
    return used_len;
  }
  std::string emsg(err);
  riiszs_free_error_message(err);
  return std::unexpected(emsg);
}

} // namespace szs
#endif

#endif /* RII_SZS_H */
