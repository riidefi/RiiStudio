#ifndef RII_BRRES_H
#define RII_BRRES_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct CResult {
  // Holds an error string if the return result of a brres_* function is not 1
  const char* json_metadata;
  uint32_t len_json_metadata;
  const void* buffer_data;
  uint32_t len_buffer_data;
  void (*freeResult)(struct CResult* self);
  void* opaque;
};
uint32_t brres_read_from_bytes(CResult* result, const void* buf, uint32_t len);
uint32_t brres_write_bytes(CResult* result, const char* json, uint32_t json_len,
                           const void* buffer, uint32_t buffer_len);
static inline void brres_free(CResult* result) {
  // There is actually a brres_free symbol that can be imported*
  if (result->freeResult) {
    result->freeResult(result);
  }
}

//! Returns the length of the string. Will be no more than 256.
int32_t brres_get_version_unstable_api(char* buf, uint32_t len);

uint32_t brres_read_mdl0mat_preset(CResult* result, const char* path,
                                   uint32_t pathLen);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#ifndef BRRES_SYS_NO_EXPECTED
#include <expected>
#endif
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace brres {

namespace impl {
template <typename TFunctor> struct Defer {
  using functor_t = TFunctor;

  TFunctor mF;
  constexpr Defer(functor_t&& F) : mF(std::move(F)) {}
  constexpr ~Defer() { mF(); }
};
} // namespace impl

struct DumpResult {
  std::string jsonData;
  std::vector<uint8_t> collatedBuffer;
};

//! Get the library version.
static inline std::string get_version() {
  std::string s;
  s.resize(1024);
  int32_t len = brres_get_version_unstable_api(&s[0], s.size());
  if (len <= 0 || len > s.size()) {
    return "Unable to query";
  }
  s.resize(len);
  return s;
}

static inline std::expected<DumpResult, std::string>
read_from_bytes(std::span<const uint8_t> bytes) {
  CResult result = {};
  uint32_t res = brres_read_from_bytes(&result, bytes.data(),
                                       static_cast<uint32_t>(bytes.size()));
  impl::Defer _([&]() { brres_free(&result); });

  if (res != 1) {
    std::string error(result.json_metadata ? result.json_metadata
                                           : "Unknown error");
    return std::unexpected(error);
  }

  DumpResult dumpResult;
  dumpResult.jsonData.assign(result.json_metadata, result.len_json_metadata);
  dumpResult.collatedBuffer.assign(
      static_cast<const uint8_t*>(result.buffer_data),
      static_cast<const uint8_t*>(result.buffer_data) + result.len_buffer_data);

  return dumpResult;
}

static inline std::expected<std::vector<u8>, std::string>
write_bytes(std::string_view json, std::span<const uint8_t> buffer) {
  CResult result = {};
  uint32_t res = brres_write_bytes(
      &result, json.data(), static_cast<uint32_t>(json.size()), buffer.data(),
      static_cast<uint32_t>(buffer.size()));
  impl::Defer _([&]() { brres_free(&result); });

  if (res != 1) {
    std::string error(result.json_metadata ? result.json_metadata
                                           : "Unknown error");
    return std::unexpected(error);
  }

  std::vector<uint8_t> buf(static_cast<const uint8_t*>(result.buffer_data),
                           static_cast<const uint8_t*>(result.buffer_data) +
                               result.len_buffer_data);

  return buf;
}

static inline std::expected<std::vector<u8>, std::string>
read_mdl0mat_folder_preset(std::string_view path) {
  CResult result = {};
  uint32_t res = brres_read_mdl0mat_preset(&result, path.data(),
                                           static_cast<uint32_t>(path.size()));
  impl::Defer _([&]() { brres_free(&result); });

  if (res != 1) {
    std::string error(result.json_metadata ? result.json_metadata
                                           : "Unknown error");
    return std::unexpected(error);
  }

  std::vector<uint8_t> buf(static_cast<const uint8_t*>(result.buffer_data),
                           static_cast<const uint8_t*>(result.buffer_data) +
                               result.len_buffer_data);

  return buf;
}

} // namespace brres
#endif

#endif /* RII_BRRES_H */
