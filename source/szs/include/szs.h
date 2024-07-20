#ifndef RII_SZS_H
#define RII_SZS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t riiszs_is_compressed(const void* src, uint32_t len);
uint32_t riiszs_decoded_size(const void* src, uint32_t len);
// YAZ0 (.szs)
const char* riiszs_decode(void* buf, uint32_t len, const void* src,
                          uint32_t src_len);
// YAY0 (.szp)
const char* riiszs_decode_yay0_into(void* buf, uint32_t len, const void* src,
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
  // Reference C implementation of Rust algorithms
  RII_SZS_ENCODE_ALGO_WORST_CASE_ENCODING_C = 100,
  RII_SZS_ENCODE_ALGO_NINTENDO_C = 101,
  LibYaz0_RustLibc = 102,
  LibYaz0_RustMemchr = 103,
  RII_SZS_ENCODE_ALGO_MK8_EXPERIMENTAL_RUST = 107,
};

const char* riiszs_encode_algo_fast(void* dst, uint32_t dst_len,
                                    const void* src, uint32_t src_len,
                                    uint32_t* used_len, uint32_t algo);
void riiszs_free_error_message(const char* msg);

int32_t szs_get_version_unstable_api(char* buf, uint32_t len);

uint32_t riiszs_deinterlaced_upper_bound(uint32_t len);
const char* riiszs_deinterlace_into(void* dst, uint32_t dst_len,
                                    const void* src, uint32_t src_len,
                                    uint32_t* used_len);

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

static inline bool is_compressed(std::span<const uint8_t> src) {
  return ::riiszs_is_compressed(src.data(), src.size());
}
static inline uint32_t decoded_size(std::span<const uint8_t> src) {
  return ::riiszs_decoded_size(src.data(), src.size());
}

static inline uint32_t encoded_upper_bound(uint32_t len) {
  return ::riiszs_encoded_upper_bound(len);
}

static inline std::expected<uint32_t, std::string>
encode_into(std::span<uint8_t> dst, std::span<const uint8_t> src, Algo algo) {
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

std::expected<std::vector<uint8_t>, std::string>
encode(std::span<const uint8_t> buf, Algo algo) {
  uint32_t worst =
      ::riiszs_encoded_upper_bound(static_cast<uint32_t>(buf.size()));
  std::vector<uint8_t> tmp(worst);
  auto ok = encode_into(tmp, buf, algo);
  if (!ok) {
    return std::unexpected(ok.error());
  }
  assert(tmp.size() >= *ok);
  tmp.resize(*ok);
  return tmp;
}

static inline std::expected<void, std::string>
decode_into(std::span<uint8_t> dst, std::span<const uint8_t> src) {
  const char* err =
      ::riiszs_decode(dst.data(), dst.size(), src.data(), src.size());
  if (err == nullptr) {
    return {};
  }
  std::string emsg(err);
  ::riiszs_free_error_message(err);
  return std::unexpected(emsg);
}
static inline std::expected<std::vector<uint8_t>, std::string>
decode(std::span<const uint8_t> src) {
  uint32_t size = ::riiszs_decoded_size(src.data(), src.size());
  std::vector<uint8_t> result(size);
  auto ok = decode_into(result, src);
  if (!ok) {
    return std::unexpected(ok.error());
  }
  return result;
}

static inline std::expected<void, std::string>
decode_yay0_into(std::span<uint8_t> dst, std::span<const uint8_t> src) {
  const char* err =
      ::riiszs_decode_yay0_into(dst.data(), dst.size(), src.data(), src.size());
  if (err == nullptr) {
    return {};
  }
  std::string emsg(err);
  ::riiszs_free_error_message(err);
  return std::unexpected(emsg);
}
static inline std::expected<std::vector<uint8_t>, std::string>
decode_yay0(std::span<const uint8_t> src) {
  uint32_t size = ::riiszs_decoded_size(src.data(), src.size());
  std::vector<uint8_t> result(size);
  auto ok = decode_yay0_into(result, src);
  if (!ok) {
    return std::unexpected(ok.error());
  }
  return result;
}

static inline std::expected<uint32_t, std::string>
deinterlace_into(std::span<uint8_t> dst, std::span<const uint8_t> src) {
  uint32_t used_len = 0;
  const char* err = ::riiszs_deinterlace_into(
      dst.data(), dst.size(), src.data(), src.size(), &used_len);
  if (err == nullptr) {
    return used_len;
  }
  std::string emsg(err);
  ::riiszs_free_error_message(err);
  return std::unexpected(emsg);
}

std::expected<std::vector<uint8_t>, std::string>
deinterlace(std::span<const uint8_t> buf) {
  uint32_t worst =
      ::riiszs_deinterlaced_upper_bound(static_cast<uint32_t>(buf.size()));
  std::vector<uint8_t> tmp(worst);
  auto ok = deinterlace_into(tmp, buf);
  if (!ok) {
    return std::unexpected(ok.error());
  }
  assert(tmp.size() >= *ok);
  tmp.resize(*ok);
  return tmp;
}

std::expected<std::vector<uint8_t>, std::string>
encode_yay0(std::span<const uint8_t> buf, Algo algo) {
  auto ok = encode(buf, algo);
  if (!ok) {
    return std::unexpected(ok.error());
  }
  auto ok2 = deinterlace(*ok);
  if (!ok2) {
    return std::unexpected(ok2.error());
  }
  return *ok2;
}

} // namespace szs
#endif

#endif /* RII_SZS_H */
