#ifndef RII_SZS_H
#define RII_SZS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t riiszs_is_compressed(const void* src, uint32_t len);
uint32_t riiszs_decoded_size(const void* src, uint32_t len);
const char* riiszs_decode(void* buf, uint32_t len, const void* src,
                          uint32_t src_len);
uint32_t riiszs_encoded_upper_bound(uint32_t len);

enum {
  RII_SZS_ENCODE_ALGO_WORST_CASE_ENCODING,
  RII_SZS_ENCODE_ALGO_NINTENDO,
  RII_SZS_ENCODE_ALGO_MKWSP,
  RII_SZS_ENCODE_ALGO_CTGP,
  RII_SZS_ENCODE_ALGO_HAROOHIE,
  RII_SZS_ENCODE_ALGO_CTLIB,
  RII_SZS_ENCODE_ALGO_LIBYAZ0,
  RII_SZS_ENCODE_ALGO_MK8,
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
#ifndef RIISZS_NO_INCLUDE_EXPECTED
#include <expected>
#endif
#include <assert.h>
#include <span>
#include <string>
#include <vector>

namespace szs {

enum class Algo {
  WorstCaseEncoding = RII_SZS_ENCODE_ALGO_WORST_CASE_ENCODING,
  Nintendo = RII_SZS_ENCODE_ALGO_NINTENDO,
  MKWSP = RII_SZS_ENCODE_ALGO_MKWSP,
  CTGP = RII_SZS_ENCODE_ALGO_CTGP,
  Haroohie = RII_SZS_ENCODE_ALGO_HAROOHIE,
  CTLib = RII_SZS_ENCODE_ALGO_CTLIB,
  LibYaz0 = RII_SZS_ENCODE_ALGO_LIBYAZ0,
  MK8 = RII_SZS_ENCODE_ALGO_MK8,
};

static inline std::string get_version() {
  std::string s;
  s.resize(1024);
  int32_t len = ::szs_get_version_unstable_api(s.data(), s.size());
  if (len <= 0 || len > s.size()) {
    return "Unable to query";
  }
  return s;
}

static inline std::expected<uint32_t, std::string>
encode_algo_fast(std::span<uint8_t> dst, std::span<const uint8_t> src,
                 Algo algo) {
  uint32_t used_len = 0;
  const uint32_t algo_u = static_cast<uint32_t>(algo);
  const char* err = ::riiszs_encode_algo_fast(
      dst.data(), dst.size(), src.data(), src.size(), &used_len, algo_u);
  if (err == nullptr) {
    return used_len;
  }
  std::string emsg(err);
  ::riiszs_free_error_message(err);
  return std::unexpected(emsg);
}

Result<std::vector<uint8_t>> encode_algo(std::span<const uint8_t> buf,
                                         Algo algo) {
  uint32_t worst = ::riiszs_encoded_upper_bound(static_cast<u32>(buf.size()));
  std::vector<uint8_t> tmp(worst);
  auto ok = encode_algo_fast(tmp, buf, algo);
  if (!ok) {
    return std::unexpected(ok.error());
  }
  assert(tmp.size() >= *ok);
  tmp.resize(*ok);
  return tmp;
}

static inline std::expected<void, std::string>
decode(std::span<uint8_t> dst, std::span<const uint8_t> src) {
  const char* err =
      ::riiszs_decode(dst.data(), dst.size(), src.data(), src.size());
  if (err == nullptr) {
    return {};
  }
  std::string emsg(err);
  ::riiszs_free_error_message(err);
  return std::unexpected(emsg);
}

} // namespace szs
#endif

#endif /* RII_SZS_H */
