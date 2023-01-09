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
#include <expected>
#include <filesystem>
#include <map>
#include <memory>
#include <numbers> // pi
#include <numeric> // accumulate is here for some reason
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#if __cplusplus > 201703L
#include <ranges>
#include <stacktrace>
#endif

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

// clang: Merged May 16 2019, Clang 9
// GCC:   Merged May 20 2021, GCC 12 (likely to release April 2022)
#ifndef __FILE_NAME__
#define __FILE_NAME__ __FILE__
#endif

#ifdef DEBUG

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>

#define DebugReport(...)                                                       \
  emscripten_log(EM_LOG_NO_PATHS | EM_LOG_CONSOLE,                             \
                 "[" __FILE_NAME__                                             \
                 ":" LIB_RII_TO_STRING(__LINE__) "] " __VA_ARGS__)

#else
#define DebugReport(...)                                                       \
  printf("[" __FILE_NAME__ ":" LIB_RII_TO_STRING(__LINE__) "] " __VA_ARGS__)
#define DebugPrint(...)                                                        \
  fprintf(stderr, "[" __FILE_NAME__ ":" LIB_RII_TO_STRING(__LINE__) "] %s\n",  \
          std::format(__VA_ARGS__).c_str());
#endif
#else
#define DebugReport(...)
#define DebugPrint(...)
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

#if defined(__clang__) || defined(__gcc__)
#define HAS_RUST_TRY
#define TRY(...)                                                               \
  ({                                                                           \
    auto y = (__VA_ARGS__);                                                    \
    if (!y) {                                                                  \
      return std::unexpected(y.error());                                       \
    }                                                                          \
    *y;                                                                        \
  })
#define BEGINTRY
#define ENDTRY
#else
template <typename T> inline auto DoTry(T&& x) {
  if (!x.has_value()) {
    fprintf(stderr, "Fatal error: %s", x.error().c_str());
    throw x.error();
  }
  return *x;
}
#define TRY(...) DoTry(__VA_ARGS__)
#define BEGINTRY try {
#define ENDTRY                                                                 \
  }                                                                            \
  catch (std::string s) {                                                      \
    return std::unexpected(s);                                                 \
  }
#endif

#define EXPECT(expr, ...)                                                      \
  if (!(expr)) [[unlikely]] {                                                  \
    auto cur = std::stacktrace::current();                                     \
    return std::unexpected("[" __FILE_NAME__                                   \
                           ":" LIB_RII_TO_STRING(__LINE__) "] " __VA_ARGS__    \
                                                           "[Internal: " #expr \
                                                           "]\n" +             \
                           std::to_string(cur));                               \
  }

#if defined(__cpp_lib_expected)
#if __cpp_lib_expected >= 202202L
template <typename T, typename E = std::string>
using Result = std::expected<T, E>;
#endif
#endif

#define RII_INTERFACE(T)                                                       \
public:                                                                        \
  virtual ~T() = default;                                                      \
                                                                               \
protected:                                                                     \
  T() = default;                                                               \
  T(const T&) = default;                                                       \
  T& operator=(const T&) = default;                                            \
  T(T&&) noexcept = default;                                                   \
  T& operator=(T&&) noexcept = default;                                        \
                                                                               \
private:
