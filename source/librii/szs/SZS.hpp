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
  Haroohie,
  CTLib,
  LibYaz0,
  MK8,
};

Result<u32> encodeAlgoFast(std::span<u8> dst, std::span<const u8> src,
                           Algo algo);
Result<std::vector<u8>> encodeAlgo(std::span<const u8> buf, Algo algo);

std::string_view szs_version();

} // namespace librii::szs
