#pragma once

#include <oishii/options.hxx>
#include <oishii/types.hxx>

#ifndef assert
#include <cassert>
#endif

// Fine as its a PCH
#include <algorithm>
#include <array>
#include <atomic>
#include <bitset>
#include <cstdio>
#include <filesystem>
#include <map>
#include <memory>
#include <numeric> // accumulate is here for some reason
#include <ranges>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// When clang-cl `import std` support, switch to these and remove above includes
// from this header
#if 1
#define IMPORT_STD                                                             \
  namespace {}

#else
#define IMPORT_STD                                                             \
import std.core;                                                             \
import std.threading;                                                        \
import std.filesystem
#endif

// Platform

// WebGL doesn't support GL_WIREFRAME
#if !defined(RII_PLATFORM_EMSCRIPTEN)
#define RII_NATIVE_GL_WIREFRAME
#endif

#define LIB_RII_TO_STRING(v) __LIB_RII_TO_STRING(v)
#define __LIB_RII_TO_STRING(v) #v

#ifdef __cplusplus
#include <cstdio>
#else
#include <stdio.h>
#endif

#ifdef DEBUG

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>

#define DebugReport(...)                                                       \
  emscripten_log(EM_LOG_NO_PATHS | EM_LOG_CONSOLE,                             \
                 "[" __FILE__                                                  \
                 ":" LIB_RII_TO_STRING(__LINE__) "] " __VA_ARGS__)

#else
#define DebugReport(...)                                                       \
  printf("[" __FILE__ ":" LIB_RII_TO_STRING(__LINE__) "] " __VA_ARGS__)
#endif
#else
#define DebugReport(...)
#endif

constexpr u32 roundDown(u32 in, u32 align) {
  return align ? in & ~(align - 1) : in;
};
constexpr u32 roundUp(u32 in, u32 align) {
  return align ? roundDown(in + (align - 1), align) : in;
};

constexpr bool is_power_of_2(u32 x) { return (x & (x - 1)) == 0; }

#define MODULE_PRIVATE public
#define MODULE_PUBLIC public

#ifdef __clang__
#define MAYBE_UNUSED [[maybe_unused]]
#else
#define MAYBE_UNUSED
#endif

namespace riistudio {
const char* translateString(std::string_view str);
} // namespace riistudio

inline const char* operator"" _j(const char* str, size_t len) {
  return riistudio::translateString({str, len});
}

#define HAS_RANGES

#ifdef __clang__
#define TRY(x)                                                                 \
  ({                                                                           \
    auto y = x;                                                                \
    if (!y.has_value()) {                                                      \
      return y.error();                                                        \
    }                                                                          \
    *y;                                                                        \
  })
#else
inline auto DoTry(auto x) {
  if (!x.has_value()) {
    fprintf(stderr, "Fatal error: %s", x.error().c_str());
    abort();
  }
  return *x;
}
#define TRY(x) DoTry(x)
#endif
