#pragma once

#include <oishii/types.hxx>
#include <oishii/options.hxx>

#ifndef assert
#include <cassert>
#endif

#define LIB_RII_TO_STRING(v) __LIB_RII_TO_STRING(v)
#define __LIB_RII_TO_STRING(v) #v

#ifdef DEBUG
#ifdef __cplusplus
#include <cstdio>
#else
#include <stdio.h>
#endif

#define DebugReport(...) printf("[" __FILE__ ":" LIB_RII_TO_STRING(__LINE__) "] " __VA_ARGS__)
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