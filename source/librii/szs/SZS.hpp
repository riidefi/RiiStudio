#pragma once

#include <core/common.h>
#include <span>
#include <vector>

namespace librii::szs {

bool isDataYaz0Compressed(std::span<const u8> src);

Result<u32> getExpandedSize(std::span<const u8> src);
Result<void> decode(std::span<u8> dst, std::span<const u8> src);

u32 getWorstEncodingSize(std::span<const u8> src);

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

std::string_view szs_version();

} // namespace librii::szs
