#pragma once

#include "Expected.hpp"
#include <string>

#if defined(__cpp_lib_expected)
#if __cpp_lib_expected >= 202202L
template <typename T, typename E = std::string>
using Result = std::expected<T, E>;
#endif
#endif
