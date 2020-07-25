#pragma once

#include <core/common.h>
#include <span>
#include <llvm/Support/Error.h>
#include <vector>

namespace riistudio::szs {

u32 getExpandedSize(const std::span<u8> src);
llvm::Error decode(std::span<u8> dst, const std::span<u8> src);
std::vector<u8> encodeFast(const std::span<u8> src);

} // namespace riistudio::szs
