#pragma once

#include "Expected.hpp"
#include "Format.hpp"
#include <string>

#if __cplusplus > 201703L
#ifndef __APPLE__
#include <stacktrace>
#endif
#endif

// clang: Merged May 16 2019, Clang 9
// GCC:   Merged May 20 2021, GCC 12 (likely to release April 2022)
#ifndef __FILE_NAME__
#define __FILE_NAME__ __FILE__
#endif

#if defined(__clang__) && !defined(__APPLE__)
#define STACK_TRACE std::stacktrace::current()
#else
#define STACK_TRACE 0
#endif

#define EXPECT(expr, ...)                                                      \
  if (!(expr)) [[unlikely]] {                                                  \
    auto cur = STACK_TRACE;                                                    \
    return std::unexpected(std::format(                                        \
        "[{}:{}] {} [Internal: {}] {}", __FILE_NAME__, __LINE__,               \
        (0 __VA_OPT__(, ) __VA_ARGS__), #expr, std::to_string(cur)));          \
  }
