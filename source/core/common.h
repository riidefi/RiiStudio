#pragma once

#include <oishii/options.hxx>
#include <oishii/types.hxx>
#include <string_view>

#ifndef assert
#include <cassert>
#endif

// Platform

// WebGL doesn't support GL_WIREFRAME
#if !defined(RII_PLATFORM_EMSCRIPTEN)
#define RII_NATIVE_GL_WIREFRAME
#endif

#define LIB_RII_TO_STRING(v) __LIB_RII_TO_STRING(v)
#define __LIB_RII_TO_STRING(v) #v

#ifdef DEBUG
#ifdef __cplusplus
#include <cstdio>
#else
#include <stdio.h>
#endif

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
