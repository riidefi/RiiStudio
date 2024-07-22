#include "SZS.hpp"

// SZS_EXPECTED: We prefer to use our definition, which may be a
// shim for tl::expected on incompatible systems*
//
// (Setting this definition prevents #include'ing the STL expected header)
#define SZS_EXPECTED std::expected
#include <szs/include/szs.h>

namespace librii::szs {

Result<std::vector<u8>> encodeAlgo(std::span<const u8> buf, Algo algo,
                                   bool yay0) {
  if (yay0) {
    return ::szs::encode_yay0(buf, static_cast<::szs::Algo>(algo));
  }
  return ::szs::encode(buf, static_cast<::szs::Algo>(algo));
}

bool isDataYaz0Compressed(std::span<const u8> src) {
  return ::szs::is_compressed(src);
}

Result<u32> getExpandedSize(std::span<const u8> src) {
  if (src.size_bytes() < 8)
    return std::unexpected("File too small to be a YAZ0/YAY0 file");

  EXPECT(src[0] == 'Y' && src[1] == 'a' && (src[2] == 'z' || src[2] == 'y') &&
         (src[3] == '0' || src[3] == '1'));
  return ::szs::decoded_size(src);
}

Result<void> decode(std::span<u8> dst, std::span<const u8> src, bool yay0) {
  if (yay0) {
    return ::szs::decode_yay0_into(dst, src);
  }
  return ::szs::decode_into(dst, src);
}

u32 getWorstEncodingSize(std::span<const u8> src) {
  return ::szs::encoded_upper_bound(static_cast<u32>(src.size()));
}

std::string_view szs_version() {
  static std::string ver = ::szs::get_version();
  return ver;
}

} // namespace librii::szs
