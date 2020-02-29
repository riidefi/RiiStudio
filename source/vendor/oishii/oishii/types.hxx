#pragma once


#ifdef  __CWCC__
typedef signed char         s8;
typedef signed short        s16;
typedef signed long         s32;
typedef signed long long    s64;
typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned long       u32;
typedef unsigned long long  u64;
#elif defined(__GNUC__) // GCC
typedef unsigned long long  u64;
typedef   signed long long  s64;
typedef unsigned int        u32;
typedef   signed int        s32;
typedef unsigned short      u16;
typedef   signed short      s16;
typedef unsigned char       u8;
typedef   signed char       s8;
#elif defined(_MSC_VER) // Win32
typedef __int8              s8;
typedef __int16             s16;
typedef __int32             s32;
typedef __int64             s64;
typedef unsigned __int8     u8;
typedef unsigned __int16    u16;
typedef unsigned __int32    u32;
typedef unsigned __int64    u64;
#endif
typedef float               f32;
typedef double              f64;

#ifdef __cplusplus
static_assert(sizeof(s8) == 1 && sizeof(u8) == 1, "!");
static_assert(sizeof(s16) == 2 && sizeof(u16) == 2, "!");
static_assert(sizeof(s32) == 4 && sizeof(u32) == 4, "!");
static_assert(sizeof(f32) == 4, "!");
static_assert(sizeof(f64) == 8, "!");
#endif
