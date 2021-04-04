#pragma once

#include <core/common.h>

#ifndef NDEBUG
#include <vector>
#else
#include <llvm/ADT/SmallVector.h>
#endif

namespace rsl {

#ifndef NDEBUG
template <typename T, unsigned N> using small_vector = std::vector<T>;
#else
template <typename T, unsigned N> using small_vector = llvm::SmallVector<T, N>;
#endif

} // namespace rsl
