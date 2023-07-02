// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

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

struct DXTBlock
{
  u16 color1;
  u16 color2;
  u8 lines[4];
};

constexpr u32 MakeRGBA(int r, int g, int b, int a)
{
  return (a << 24) | (b << 16) | (g << 8) | r;
}

constexpr int DXTBlend(int v1, int v2)
{
  // 3/8 blend, which is close to 1/3
  return ((v1 * 3 + v2 * 5) >> 3);
}
