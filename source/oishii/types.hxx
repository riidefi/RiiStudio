#pragma once

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

#ifdef __cplusplus
static_assert(sizeof(s8) == 1 && sizeof(u8) == 1, "!");
static_assert(sizeof(s16) == 2 && sizeof(u16) == 2, "!");
static_assert(sizeof(s32) == 4 && sizeof(u32) == 4, "!");
static_assert(sizeof(f32) == 4, "!");
static_assert(sizeof(f64) == 8, "!");
#endif

#ifndef _WIN32
#define __debugbreak(...)
#endif
