#include "WBZ.hpp"

#define CWBZ_NO_INCLUDE_EXPECTED
#include "c-wbz/include/wbz_rs.h"

namespace librii::wbz {

Result<std::vector<u8>> decodeWBZ(std::span<const u8> buf,
                                  std::string_view autoadd_path) {
  auto ok = wbz_rs::decode_wbz(buf, autoadd_path);
  if (!ok) {
    return std::unexpected(wbz_rs::error_to_string(ok.error()));
  }
  return *ok;
}

Result<void> decodeWU8Inplace(std::span<u8> buf,
                              std::string_view autoadd_path) {
  auto ok = wbz_rs::decode_wu8_inplace(buf, autoadd_path);
  if (!ok) {
    return std::unexpected(wbz_rs::error_to_string(ok.error()));
  }
  return {};
}
Result<void> encodeWU8Inplace(std::span<u8> buf,
                              std::string_view autoadd_path) {
  auto ok = wbz_rs::encode_wu8_inplace(buf, autoadd_path);
  if (!ok) {
    return std::unexpected(wbz_rs::error_to_string(ok.error()));
  }
  return {};
}

Result<std::vector<u8>> encodeWBZ(std::span<const u8> buf,
                                  std::string_view autoadd_path) {
  auto ok = wbz_rs::encode_wbz(buf, autoadd_path);
  if (!ok) {
    return std::unexpected(wbz_rs::error_to_string(ok.error()));
  }
  return *ok;
}

} // namespace librii::wbz
