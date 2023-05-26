#pragma once

#include <oishii/options.hxx>
#include <stdint.h>

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

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
#include <functional>
#include <map>
#include <memory>
#include <numbers> // pi
#include <numeric> // accumulate is here for some reason
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <rsl/Try.hpp>

#if __cplusplus > 201703L
#include <ranges>
#ifndef __APPLE__
#include <stacktrace>
#endif
#endif

// No <ranges> yet
#ifdef __APPLE__
#include <range/v3/algorithm/adjacent_find.hpp>
#include <range/v3/algorithm/any_of.hpp>
#include <range/v3/algorithm/count.hpp>
#include <range/v3/algorithm/equal.hpp>
#include <range/v3/algorithm/fill.hpp>
#include <range/v3/algorithm/find.hpp>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/algorithm/max_element.hpp>
#include <range/v3/algorithm/min_element.hpp>
#include <range/v3/algorithm/sort.hpp>
#include <range/v3/view/drop.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/reverse.hpp>
#include <range/v3/view/split.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/view/transform.hpp>
namespace std {
namespace views {
using namespace ::ranges::views;
}
namespace ranges {
using namespace ::ranges;
}
} // namespace std
#endif

#if defined(__APPLE__) || defined(__GCC__) || defined(__GNUC__)
#include <fmt/format.h>
namespace std {
using namespace fmt;
}
#else
#include <format>
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

#if (defined(__APPLE__) && __cplusplus > 201703L) ||                           \
    (!defined(__APPLE__) && __cpp_lib_expected >= 202202L)
namespace {
template <typename A, typename B> int indexOf(A&& x, B&& y) {
  int index = std::find_if(x.begin(), x.end(),
                           [y](auto& f) { return f.getName() == y; }) -
              x.begin();
  return index >= x.size() ? -1 : index;
};
auto findByName = [](auto&& x, auto&& y) {
  int index = std::find_if(x.begin(), x.end(),
                           [y](auto& f) { return f.getName() == y; }) -
              x.begin();
  return index >= x.size() ? nullptr : &x[index];
};
auto findByName2 = [](auto&& x, auto&& y) {
  int index =
      std::find_if(x.begin(), x.end(), [y](auto& f) { return f.name == y; }) -
      x.begin();
  return index >= x.size() ? nullptr : &x[index];
};
} // namespace
#endif

#include <rsl/Log.hpp>
