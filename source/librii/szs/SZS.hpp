#pragma once

#include <core/common.h>
#include <span>
#include <vector>

namespace librii::szs {

Result<u32> getExpandedSize(std::span<const u8> src);
Result<void> decode(std::span<u8> dst, std::span<const u8> src);

u32 getWorstEncodingSize(std::span<const u8> src);
std::vector<u8> encodeFast(std::span<const u8> src);

int encodeBoyerMooreHorspool(const u8* src, u8* dst, int srcSize);

} // namespace librii::szs
