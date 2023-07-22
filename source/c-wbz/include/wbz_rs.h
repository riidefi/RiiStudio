#pragma once

#include <cstdlib>
#include <cstring>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  WBZ_ERR_OK = 0,
  WBZ_ERR_BZIP,
  WBZ_ERR_FILE_TOO_BIG,
  WBZ_ERR_FILE_OPERATION_FAILED,
  WBZ_ERR_INVALID_WBZ_MAGIC,
  WBZ_ERR_INVALID_WU8_MAGIC,
  WBZ_ERR_INVALID_STRING,
  WBZ_ERR_INVALID_BOOL,
} wbzrs_errc;

typedef struct {
  void* data;
  uint32_t size;
} wbzrs_buffer;

int32_t wbzrs_decode_wbz(const void* wbz_buffer, uint32_t wbz_len,
                         const char* autoadd_path, wbzrs_buffer* decoded);
int32_t wbzrs_decode_wu8(void* wu8_buffer, uint32_t wu8_len,
                         const char* autoadd_path);

const char* wbzrs_error_to_string(int32_t ec);
void wbzrs_free_buffer(wbzrs_buffer* buf);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace wbz_rs {

enum class Error {
  BZIP = 1,
  FILE_TOO_BIG,
  FILE_OPERATION_FAILED,
  INVALID_WBZ_MAGIC,
  INVALID_WU8_MAGIC,
  INVALID_STRING,
  INVALID_BOOL,
};

static inline const char* error_to_string(Error errc) {
  return wbzrs_error_to_string(static_cast<int32_t>(errc));
}

static inline std::expected<std::vector<uint8_t>, Error>
decode_wbz(std::span<const uint8_t> buf, std::string_view autoadd_path) {
  std::string autoadd_path_c(autoadd_path);
  wbzrs_buffer decoded{};
  int32_t errc = wbzrs_decode_wbz(buf.data(), buf.size(),
                                  autoadd_path_c.c_str(), &decoded);
  if (errc != WBZ_ERR_OK) {
    return std::unexpected(static_cast<Error>(errc));
  }
  uint8_t* begin = reinterpret_cast<uint8_t*>(decoded.data);
  std::vector<uint8_t> result(begin, begin + decoded.size);
  wbzrs_free_buffer(&decoded);
  return result;
}

static inline std::expected<void, Error>
decode_wu8(std::span<uint8_t> buf, std::string_view autoadd_path) {
  std::string autoadd_path_c(autoadd_path);
  int32_t errc =
      wbzrs_decode_wu8(buf.data(), buf.size(), autoadd_path_c.c_str());
  if (errc != WBZ_ERR_OK) {
    return std::unexpected(static_cast<Error>(errc));
  }
  return {};
}

} // namespace wbz_rs
#endif // __cplusplus
