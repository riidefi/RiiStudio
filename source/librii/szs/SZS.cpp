#include "SZS.hpp"

#include <szs/include/szs.h>

namespace librii::szs {

Result<u32> encodeAlgoFast(std::span<u8> dst, std::span<const u8> src,
                           Algo algo) {
  uint32_t algo_u = static_cast<uint32_t>(algo);
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
  EXPECT(dst.size() >= TRY(getExpandedSize(src)));

  int in_position = 0x10;
  int out_position = 0;

  const auto take8 = [&]() {
    const u8 result = src[in_position];
    ++in_position;
    return result;
  };
  const auto take16 = [&]() {
    const u32 high = src[in_position];
    ++in_position;
    const u32 low = src[in_position];
    ++in_position;
    return (high << 8) | low;
  };
  const auto give8 = [&](u8 val) {
    dst[out_position] = val;
    ++out_position;
  };

  const auto read_group = [&](bool raw) {
    if (raw) {
      give8(take8());
      return;
    }

    const u16 group = take16();
    const int reverse = (group & 0xfff) + 1;
    const int g_size = group >> 12;

    int size = g_size ? g_size + 2 : take8() + 18;

    // Invalid data could buffer overflow

    for (int i = 0; i < size; ++i) {
      give8(dst[out_position - reverse]);
    }
  };

  const auto read_chunk = [&]() {
    const u8 header = take8();

    for (int i = 0; i < 8; ++i) {
      if (in_position >= src.size() || out_position >= dst.size())
        return;
      read_group(header & (1 << (7 - i)));
    }
  };

  while (in_position < src.size() && out_position < dst.size())
    read_chunk();

  // if (out_position < dst.size())
  //   return llvm::createStringError(
  //       std::errc::executable_format_error,
  //       "Truncated source file: the file could not be decompressed fully");

  // if (in_position != src.size())
  //   return llvm::createStringError(
  //       std::errc::no_buffer_space,
  //       "Invalid YAZ0 header: file is larger than reported");

  return {};
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
  std::span<const u8> ss {src, srcSize};
  std::span<u8> ds{dst, dstSize};
  return *encodeAlgoFast(ds, ss, Algo::MkwSp);
}

std::string_view szs_version() {
  static std::string ver = ::szs::get_version();
  return ver;
}

} // namespace librii::szs
