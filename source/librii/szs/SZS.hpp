#pragma once

#include <core/common.h>
#include <span>
#include <vector>

namespace librii::szs {

bool isDataYaz0Compressed(std::span<const u8> src);

Result<u32> getExpandedSize(std::span<const u8> src);
Result<void> decode(std::span<u8> dst, std::span<const u8> src);

u32 getWorstEncodingSize(std::span<const u8> src);
std::vector<u8> encodeFast(std::span<const u8> src);

int encodeBoyerMooreHorspool(const u8* src, u8* dst, int srcSize);

u32 encodeSP(const u8* src, u8* dst, u32 srcSize, u32 dstSize);
Result<std::vector<u8>> encodeCTGP(std::span<const u8> buf);

enum class Algo {
  WorstCaseEncoding,
  Nintendo,
  MkwSp,
  CTGP,
};

static inline Result<std::vector<u8>> encodeAlgo(std::span<const u8> buf,
                                                 Algo algo) {
  if (algo == Algo::WorstCaseEncoding) {
    return encodeFast(buf);
  }
  if (algo == Algo::Nintendo) {
    std::vector<u8> tmp(getWorstEncodingSize(buf));
    int sz = encodeBoyerMooreHorspool(buf.data(), tmp.data(), buf.size());
    if (sz < 0 || sz > tmp.size()) {
      return std::unexpected("encodeBoyerMooreHorspool failed");
    }
    tmp.resize(sz);
    return tmp;
  }
  if (algo == Algo::MkwSp) {
    std::vector<u8> tmp(getWorstEncodingSize(buf));
    u32 sz = encodeSP(buf.data(), tmp.data(), buf.size(), tmp.size());
    if (sz > tmp.size()) {
      return std::unexpected("encodeSP failed");
    }
    tmp.resize(sz);
    return tmp;
  }
  if (algo == Algo::CTGP) {
    return encodeCTGP(buf);
  }

  return std::unexpected("Invalid algorithm");
}

} // namespace librii::szs
