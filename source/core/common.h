#pragma once

#include <rsl/Types.hpp>

#ifndef assert
#include <cassert>
#endif

// Fine as its a PCH
#include <array>
#include <bitset>
#include <cstdio>
#include <functional>
#include <map>
#include <memory>
#include <numbers> // pi
#include <numeric> // accumulate is here for some reason
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <rsl/Expect.hpp>
#include <rsl/Result.hpp>
#include <rsl/Try.hpp>

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

constexpr u32 roundDown(u32 in, u32 align) {
  return align ? in & ~(align - 1) : in;
};
constexpr u32 roundUp(u32 in, u32 align) {
  return align ? roundDown(in + (align - 1), align) : in;
};

constexpr bool is_power_of_2(u32 x) { return (x & (x - 1)) == 0; }

namespace riistudio {
const char* translateString(std::string_view str);
} // namespace riistudio

inline const char* operator"" _j(const char* str, size_t len) {
  return riistudio::translateString({str, len});
}

#define HAS_RANGES

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

#include <rsl/Log.hpp>
