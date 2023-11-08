#pragma once

#include "expected.hpp"

#include <span>
#include <stdint.h>
#include <string>
#include <vector>

#include "util.h"

constexpr static inline u32 roundDown(u32 in, u32 align) {
  return align ? in & ~(align - 1) : in;
}
constexpr static inline u32 roundUp(u32 in, u32 align) {
  return align ? roundDown(in + (align - 1), align) : in;
}

static inline void writeU16(u8* data, u32 offset, u16 val) {
  u8* base = data + offset;
  base[0x0] = val >> 8;
  base[0x1] = val;
}

static inline void writeU32(u8* data, u32 offset, u32 val) {
  u8* base = data + offset;
  base[0x0] = val >> 24;
  base[0x1] = val >> 16;
  base[0x2] = val >> 8;
  base[0x3] = val;
}

namespace rlibrii::szs {

template <typename T> using Result = tl::expected<T, std::string>;

bool isDataYaz0Compressed(std::span<const u8> src);

Result<u32> getExpandedSize(std::span<const u8> src);
Result<void> decode(std::span<u8> dst, std::span<const u8> src);

u32 getWorstEncodingSize(u32 src);
u32 getWorstEncodingSize(std::span<const u8> src);
std::vector<u8> encodeFast(std::span<const u8> src);

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

enum {
  YAZ0_MAGIC = 0x59617a30,
  YAZ1_MAGIC = 0x59617a31,
};

} // namespace rlibrii::szs
