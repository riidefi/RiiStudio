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
  RII_SZS_ENCODE_ALGO_MKWSP, // Legacy
  RII_SZS_ENCODE_ALGO_CTGP,
  RII_SZS_ENCODE_ALGO_HAROOHIE, // Legacy
  RII_SZS_ENCODE_ALGO_CTLIB,    // Legacy
  RII_SZS_ENCODE_ALGO_LIBYAZ0,
  RII_SZS_ENCODE_ALGO_MK8,
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

#include <span>
#include <string>
#include <vector>

#ifndef SZS_EXPECTED
#include <expected>
#define SZS_EXPECTED std::expected
#endif

#ifndef SZS_ASSERT
#include <assert.h>
#define SZS_ASSERT assert
#endif

namespace szs {

namespace impl {
static inline std::string rust_string(const char* s) {
  SZS_ASSERT(s != nullptr);
  std::string emsg(s);
  ::riiszs_free_error_message(s);
  return emsg;
}
} // namespace impl

template <typename T> using Result = SZS_EXPECTED<T, std::string>;

enum class Algo {
  //! INSTANT encoding
  WorstCaseEncoding = RII_SZS_ENCODE_ALGO_WORST_CASE_ENCODING,

  //! N64-Wii matching algorithm
  Nintendo = RII_SZS_ENCODE_ALGO_NINTENDO,

  //! Brute-force, best-case algorithm
  LibYaz0 = RII_SZS_ENCODE_ALGO_LIBYAZ0,

  //! FAST encoding
  MK8 = RII_SZS_ENCODE_ALGO_MK8,

  //! Matching CTGP algorithm
  CTGP = RII_SZS_ENCODE_ALGO_CTGP,

  // Legacy algorithms...
  MKWSP = RII_SZS_ENCODE_ALGO_MKWSP,
  Haroohie = RII_SZS_ENCODE_ALGO_HAROOHIE,
  CTLib = RII_SZS_ENCODE_ALGO_CTLIB,
};

//! Get the API version (e.g. `riidefi/gctex 0.3.7`)
static inline std::string get_version() {
  std::string s;
  s.resize(1024);
  int32_t len = ::szs_get_version_unstable_api(s.data(), s.size());
  if (len <= 0 || len > s.size()) {
    return "Unable to query";
  }
  return s;
}

//! Is a blob YAZ0/YAY0 compressed?
static inline bool is_compressed(std::span<const uint8_t> src) {
  return ::riiszs_is_compressed(src.data(), src.size());
}
//! What is the decoded size of an archive?
static inline uint32_t decoded_size(std::span<const uint8_t> src) {
  return ::riiszs_decoded_size(src.data(), src.size());
}
//! Amount of bytes to allocate for `encode_into` API.
static inline uint32_t encoded_upper_bound(uint32_t len) {
  return ::riiszs_encoded_upper_bound(len);
}

//! Directly encode into a buffer.
//! @see `szs::encoded_upper_bound`
static inline Result<uint32_t>
encode_into(std::span<uint8_t> dst, std::span<const uint8_t> src, Algo algo) {
  uint32_t used_len = 0;
  const uint32_t algo_u = static_cast<uint32_t>(algo);
  const char* err = ::riiszs_encode_algo_fast(
      dst.data(), dst.size(), src.data(), src.size(), &used_len, algo_u);
  if (err == nullptr) {
    return used_len;
  }
  return std::unexpected(impl::rust_string(err));
}

//! Encode a buffer as YAZ0 data.
static inline Result<std::vector<uint8_t>> encode(std::span<const uint8_t> buf,
                                                  Algo algo) {
  uint32_t worst =
      ::riiszs_encoded_upper_bound(static_cast<uint32_t>(buf.size()));
  std::vector<uint8_t> tmp(worst);
  auto ok = encode_into(tmp, buf, algo);
  if (!ok) {
    return std::unexpected(ok.error());
  }
  SZS_ASSERT(tmp.size() >= *ok);
  tmp.resize(*ok);
  return tmp;
}

//! Directly decode into a buffer.
//! @see `szs::decoded_size`
static inline Result<void> decode_into(std::span<uint8_t> dst,
                                       std::span<const uint8_t> src) {
  const char* err =
      ::riiszs_decode(dst.data(), dst.size(), src.data(), src.size());
  if (err == nullptr) {
    return {};
  }
  return std::unexpected(impl::rust_string(err));
}

//! Decode a YAZ0 file.
static inline Result<std::vector<uint8_t>>
decode(std::span<const uint8_t> src) {
  uint32_t size = ::riiszs_decoded_size(src.data(), src.size());
  std::vector<uint8_t> result(size);
  auto ok = decode_into(result, src);
  if (!ok) {
    return std::unexpected(ok.error());
  }
  return result;
}

//! Decode a YAY0 file directly into a buffer.
static inline Result<void> decode_yay0_into(std::span<uint8_t> dst,
                                            std::span<const uint8_t> src) {
  const char* err =
      ::riiszs_decode_yay0_into(dst.data(), dst.size(), src.data(), src.size());
  if (err == nullptr) {
    return {};
  }
  return std::unexpected(impl::rust_string(err));
}

//! Decoide a YAY0 file.
static inline Result<std::vector<uint8_t>>
decode_yay0(std::span<const uint8_t> src) {
  uint32_t size = ::riiszs_decoded_size(src.data(), src.size());
  std::vector<uint8_t> result(size);
  auto ok = decode_yay0_into(result, src);
  if (!ok) {
    return std::unexpected(ok.error());
  }
  return result;
}

//! YAZ0 -> YAY0 into a buffer.
static inline Result<uint32_t> deinterlace_into(std::span<uint8_t> dst,
                                                std::span<const uint8_t> src) {
  uint32_t used_len = 0;
  const char* err = ::riiszs_deinterlace_into(
      dst.data(), dst.size(), src.data(), src.size(), &used_len);
  if (err == nullptr) {
    return used_len;
  }
  return std::unexpected(impl::rust_string(err));
}

//! YAZ0 -> YAY0
static inline Result<std::vector<uint8_t>>
deinterlace(std::span<const uint8_t> buf) {
  uint32_t worst =
      ::riiszs_deinterlaced_upper_bound(static_cast<uint32_t>(buf.size()));
  std::vector<uint8_t> tmp(worst);
  auto ok = deinterlace_into(tmp, buf);
  if (!ok) {
    return std::unexpected(ok.error());
  }
  SZS_ASSERT(tmp.size() >= *ok);
  tmp.resize(*ok);
  return tmp;
}

//! Encode a YAY0 file into a buffer.
static inline Result<uint32_t> encode_yay0_into(std::span<uint8_t> dst,
                                                std::span<const uint8_t> buf,
                                                Algo algo) {
  auto ok = encode(buf, algo);
  if (!ok) {
    return std::unexpected(ok.error());
  }
  auto ok2 = deinterlace_into(dst, *ok);
  if (!ok2) {
    return std::unexpected(ok2.error());
  }
  return *ok2;
}

//! Encode a YAY0 file.
static inline Result<std::vector<uint8_t>>
encode_yay0(std::span<const uint8_t> buf, Algo algo) {
  uint32_t yaz0_max =
      ::riiszs_encoded_upper_bound(static_cast<uint32_t>(buf.size()));
  uint32_t yay0_max = ::riiszs_deinterlaced_upper_bound(yaz0_max);
  std::vector<uint8_t> tmp(yay0_max);
  auto ok = encode_yay0_into(tmp, buf, algo);
  if (!ok) {
    return std::unexpected(ok.error());
  }
  SZS_ASSERT(tmp.size() >= *ok);
  tmp.resize(*ok);
  return tmp;
}

} // namespace szs
#endif

#endif /* RII_SZS_H */
