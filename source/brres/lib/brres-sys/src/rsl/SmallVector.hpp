#pragma once

#include <core/common.h>

#if 1
#include <vector>
#else
#include <llvm/ADT/SmallVector.h>
#endif

namespace rsl {

#if 1
template <typename T, unsigned N> using small_vector = std::vector<T>;
#else
template <typename T, unsigned N> using small_vector = llvm::SmallVector<T, N>;
#endif

} // namespace rsl
