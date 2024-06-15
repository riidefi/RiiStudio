#pragma once

#include <core/common.h>

#ifndef NDEBUG
#include <vector>
#else
#include "SmallVectorImpl.hpp"
#endif

namespace rsl {

#ifndef NDEBUG
template <typename T, unsigned N> using small_vector = std::vector<T>;
#else
template <typename T, unsigned N> using small_vector = rsl::SmallVector<T, N>;
#endif

} // namespace rsl
