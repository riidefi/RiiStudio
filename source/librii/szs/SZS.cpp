#include "SZS.hpp"

#define RIISZS_NO_INCLUDE_EXPECTED
#include <szs/include/szs.h>

namespace librii::szs {

Result<u32> encodeAlgoFast(std::span<u8> dst, std::span<const u8> src,
                           Algo algo) {
  auto algo_u = static_cast<::szs::Algo>(algo);
  return ::szs::encode_algo_fast(dst, src, algo_u);
}

Result<std::vector<u8>> encodeAlgo(std::span<const u8> buf, Algo algo) {
  uint32_t worst = getWorstEncodingSize(buf);
  std::vector<u8> tmp(worst);
  auto ok = encodeAlgoFast(tmp, buf, algo);
  if (!ok) {
    return std::unexpected(ok.error());
  }
  EXPECT(tmp.size() >= *ok);
  tmp.resize(*ok);
  return tmp;
}

Result<std::vector<u8>> encodeCTGP(std::span<const u8> buf) {
  return encodeAlgo(buf, Algo::CTGP);
}

bool isDataYaz0Compressed(std::span<const u8> src) {
  if (src.size_bytes() < 8)
    return false;

  return src[0] == 'Y' && src[1] == 'a' && src[2] == 'z' && src[3] == '0';
}

Result<u32> getExpandedSize(std::span<const u8> src) {
  if (src.size_bytes() < 8)
    return std::unexpected("File too small to be a YAZ0 file");

  EXPECT(src[0] == 'Y' && src[1] == 'a' && src[2] == 'z' && src[3] == '0');
  return (src[4] << 24) | (src[5] << 16) | (src[6] << 8) | src[7];
}

Result<void> decode(std::span<u8> dst, std::span<const u8> src) {
  return ::szs::decode(dst, src);
}

u32 getWorstEncodingSize(std::span<const u8> src) {
  return 16 + roundUp(src.size(), 8) / 8 * 9 - 1;
}
std::vector<u8> encodeFast(std::span<const u8> src) {
  return *encodeAlgo(src, Algo::WorstCaseEncoding);
}

int encodeBoyerMooreHorspool(const u8* src, u8* dst, int srcSize) {
  assert(srcSize >= 0);
  std::span<const u8> ss{src, static_cast<size_t>(srcSize)};
  std::span<u8> ds{dst, getWorstEncodingSize(ss)};
  return *encodeAlgoFast(ds, ss, Algo::Nintendo);
}

u32 encodeSP(const u8* src, u8* dst, u32 srcSize, u32 dstSize) {
  std::span<const u8> ss{src, srcSize};
  std::span<u8> ds{dst, dstSize};
  return *encodeAlgoFast(ds, ss, Algo::MkwSp);
}

std::string_view szs_version() {
  static std::string ver = ::szs::get_version();
  return ver;
}

} // namespace librii::szs
