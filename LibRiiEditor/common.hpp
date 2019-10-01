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

#ifdef __cplusplus
template<typename T, int inc = 1>
struct ScopedInc
{
	inline ScopedInc(T& v_)
		: v(v_)
	{}
	inline ScopedInc()
	{ v += inc; }

	T& v;
};

template<typename T, int inc = 1>
using ScopedDec = ScopedInc<T, -inc>;

#endif
