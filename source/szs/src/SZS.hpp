#pragma once

#include "expected.hpp"

#include <span>
#include <stdint.h>
#include <string>
#include <vector>

using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;
using s32 = int32_t;
using s16 = int16_t;
using s8 = int8_t;

constexpr u32 roundDown(u32 in, u32 align) {
  return align ? in & ~(align - 1) : in;
};
constexpr u32 roundUp(u32 in, u32 align) {
  return align ? roundDown(in + (align - 1), align) : in;
};

namespace rlibrii::szs {

template <typename T> using Result = tl::expected<T, std::string>;

bool isDataYaz0Compressed(std::span<const u8> src);

Result<u32> getExpandedSize(std::span<const u8> src);
Result<void> decode(std::span<u8> dst, std::span<const u8> src);

u32 getWorstEncodingSize(u32 src);
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

Result<std::vector<u8>> encodeAlgo(std::span<const u8> buf, Algo algo);

void CompressYaz(const u8* src_, u32 src_len, u8 opt_compr, u8* dest,
                 u32 dest_len, u32* out_len);

} // namespace rlibrii::szs
