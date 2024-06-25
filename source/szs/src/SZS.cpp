#include "SZS.hpp"

#include "CTGP.hpp"
#include "CTLib.hpp"
#include "HaroohieYaz0.hpp"
#include "LibYaz.hpp"
#include "MK8.hpp"
#include "MKW.hpp"
#include "SP.hpp"

#include <algorithm>
#include <string.h>

namespace rlibrii::szs {

Result<std::vector<u8>> encodeAlgo(std::span<const u8> buf, Algo algo) {
  if (algo == Algo::WorstCaseEncoding) {
    return encodeFast(buf);
  }
  if (algo == Algo::Nintendo) {
    std::vector<u8> tmp(getWorstEncodingSize(buf));
    int sz = encodeBoyerMooreHorspool(buf.data(), tmp.data(), buf.size());
    if (sz < 0 || sz > tmp.size()) {
      return tl::unexpected("encodeBoyerMooreHorspool failed");
    }
    tmp.resize(sz);
    return tmp;
  }
  if (algo == Algo::MkwSp) {
    std::vector<u8> tmp(getWorstEncodingSize(buf));
    u32 sz = encodeSP(buf.data(), tmp.data(), buf.size(), tmp.size());
    if (sz > tmp.size()) {
      return tl::unexpected("encodeSP failed");
    }
    tmp.resize(sz);
    return tmp;
  }
  if (algo == Algo::CTGP) {
    return encodeCTGP(buf);
  }
  if (algo == Algo::Haroohie) {
    return HaroohiePals::Yaz0::Compress(buf);
  }
  if (algo == Algo::CTLib) {
    return CTLib::compressBase(buf);
  }
  if (algo == Algo::LibYaz0) {
    std::vector<u8> tmp(getWorstEncodingSize(buf));
    u32 sz = 0;
    CompressYaz(buf.data(), buf.size(), 9, tmp.data(), tmp.size(), &sz);
    tmp.resize(sz);
    return tmp;
  }
  if (algo == Algo::MK8) {
    std::vector<u8> tmp(getWorstEncodingSize(buf));
    u32 sz = CompressMK8(buf, tmp);
    tmp.resize(sz);
    return tmp;
  }

  return tl::unexpected("Invalid algorithm: id=" + std::to_string((int)algo));
}

bool isDataYaz0Compressed(std::span<const u8> src) {
  if (src.size_bytes() < 8)
    return false;

  return src[0] == 'Y' && src[1] == 'a' && src[2] == 'z' && src[3] == '0';
}

Result<u32> getExpandedSize(std::span<const u8> src) {
  if (src.size_bytes() < 8)
    return tl::unexpected("File too small to be a YAZ0 file");

  if (!(src[0] == 'Y' && src[1] == 'a' && src[2] == 'z' && src[3] == '0')) {
    return tl::unexpected("Data is not a YAZ0 file");
  }
  return (src[4] << 24) | (src[5] << 16) | (src[6] << 8) | src[7];
}

Result<void> decode(std::span<u8> dst, std::span<const u8> src) {
  auto exp = getExpandedSize(src);
  if (!exp) {
    return tl::unexpected("Source is not a SZS compressed file!");
  }
  if (dst.size() < *exp) {
    return tl::unexpected("Result buffer is too small!");
  }

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

u32 getWorstEncodingSize(u32 src) { return 16 + roundUp(src, 8) / 8 * 9 - 1; }
u32 getWorstEncodingSize(std::span<const u8> src) {
  return getWorstEncodingSize(static_cast<u32>(src.size()));
}
std::vector<u8> encodeFast(std::span<const u8> src) {
  std::vector<u8> result(getWorstEncodingSize(src));

  result[0] = 'Y';
  result[1] = 'a';
  result[2] = 'z';
  result[3] = '0';

  result[4] = (src.size() & 0xff00'0000) >> 24;
  result[5] = (src.size() & 0x00ff'0000) >> 16;
  result[6] = (src.size() & 0x0000'ff00) >> 8;
  result[7] = (src.size() & 0x0000'00ff) >> 0;

  std::fill(result.begin() + 8, result.begin() + 16, 0);

// Old implementation: Marginally slower but easier to read/reason about
#if 0
  auto* dst = result.data() + 16;

  unsigned counter = 0;
  for (auto* it = src.data(); it < src.data() + src.size(); --counter) {
    if (!counter) {
      *dst++ = 0xff;
      counter = 8;
    }
    *dst++ = *it++;
  }
  while (counter--)
    *dst++ = 0;
#else
  // New implementation courtesy abood

  auto* dst_pos = result.data() + 16;
  auto* src_pos = src.data();
  auto* src_end = src.data() + src.size();

  while (src_end - src_pos >= 8) {
    *dst_pos++ = 0xFF;
    memcpy(dst_pos, src_pos, 8);
    dst_pos += 8;
    src_pos += 8;
  }

  u32 delta = src_end - src_pos;
  if (delta > 0) {
    *dst_pos = ((1 << delta) - 1) << (8 - delta);
    memcpy(dst_pos + 1, src_pos, delta); // +1 to skip the marker byte
    dst_pos += delta + 1;                // +1 to account for the marker byte
    src_pos += delta;
  }
#endif

  return result;
}

} // namespace rlibrii::szs
