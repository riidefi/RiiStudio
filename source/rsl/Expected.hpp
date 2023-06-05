#pragma once

// As AppleClang doesn't support C++23 std::expected yet.
#if __cpp_lib_expected >= 202202L
#include <expected>
#else
#ifdef __APPLE__
#error "Please update to XCode 15"
#else
#error "Unsupported compiler version: must support std::expected"
#endif
#endif
