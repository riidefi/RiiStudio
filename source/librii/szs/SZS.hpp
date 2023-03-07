#pragma once

#include <core/common.h>
#include <llvm/Support/Error.h>
#include <span>
#include <vector>

namespace librii::szs {

// 0 if invalid
u32 getExpandedSize(std::span<const u8> src);
llvm::Error decode(std::span<u8> dst, std::span<const u8> src);
std::vector<u8> encodeFast(std::span<const u8> src);

int encodeBoyerMooreHorspool(const u8* src, u8* dst, int srcSize);

} // namespace librii::szs
