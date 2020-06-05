/*!
 * @file
 * @brief Utilities for endian streams.
 */

#pragma once

#include "../options.hxx"
#include "../types.hxx"


namespace oishii {
// Console colors

#ifdef _MSC_VER
#define IF_WIN(x) x
#include <windows.h>
#else
#define IF_WIN(x)
#endif

struct Console
{
	IF_WIN(HANDLE handle);
	IF_WIN(CONSOLE_SCREEN_BUFFER_INFO prior_state);
	IF_WIN(u16 last); // last attribute

	void restoreFirstColorState()
	{
		IF_WIN(SetConsoleTextAttribute(handle, prior_state.wAttributes));
	}

	// Note: last is a single value, not a stack -- cannot nest 
	// for this reason, deprecated

//	void restoreLastColorState()
//	{
//		setColorState(last);
//	}
	void setColorState(u16 attrib)
	{
		IF_WIN(last = attrib);
		IF_WIN(SetConsoleTextAttribute(handle, attrib));
	}

#ifdef _MSC_VER
	Console() : handle(GetStdHandle(STD_OUTPUT_HANDLE)) {
		GetConsoleScreenBufferInfo(handle, &prior_state);
		last = prior_state.wAttributes;
	}
	~Console() {
		restoreFirstColorState();
	}
#endif
	static Console sInstance;
	static Console& getInstance() { return sInstance; }
};
#ifndef _MSC_VER
struct ScopedFormatter
{
	ScopedFormatter(u16) {}
};
#else
struct ScopedFormatter
{
	u16 last;

	ScopedFormatter(u16 attrib)
	{
		last = Console::getInstance().last;
		Console::getInstance().setColorState(attrib);
	}
	~ScopedFormatter()
	{
		Console::getInstance().setColorState(last);
	}
};
#endif


template<typename T1, typename T2>
union enumCastHelper
{
	// Switch above means this always is instantiated
	// static_assert(sizeof(T1) == sizeof(T2), "Sizes of types must match.");
	T1 _t;
	T2 _u;

	enumCastHelper(T1 _)
		: _t(_)
	{}
};

static inline u32 swap32(u32 v);
static inline u16 swap16(u16 v);

#define _BSWAP_16(v) (((v & 0xff00) >> 8) | \
					  ((v & 0x00ff) << 8))

#define _BSWAP_32(v) (((v & 0xff000000) >> 24) | \
					  ((v & 0x00ff0000) >>  8) | \
					  ((v & 0x0000ff00) <<  8) | \
					  ((v & 0x000000ff) << 24) )

#if OISHII_PLATFORM_LE == 1
#define MAKE_BE32(x) _BSWAP_32(x)
#define MAKE_LE32(x) x
#else
#define MAKE_BE32(x) x
#define MAKE_LE32(x) _BSWAP_32(x)
#endif

#ifdef _MSC_VER
#include <stdlib.h>
static inline u32 swap32(u32 v)
{
	return _byteswap_ulong(v);
}
static inline u16 swap16(u16 v)
{
	return _byteswap_ushort(v);
}
#else
static inline u32 swap32(u32 v)
{
	return _BSWAP_32(v);
}
static inline u16 swap16(u16 v)
{
	return _BSWAP_16(v);
}
#endif




//! @brief Fast endian swapping.
//!
//! @tparam T Type of value to swap. Must be sized 1, 2, or 4
//!
//! @return T endian swapped.
template<typename T>
inline T swapEndian(T v)
{
	static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4, "T must of size 1, 2, or 4");

	switch (sizeof(T))
	{
	case 4:
	{
		enumCastHelper<T, u32> tmp(v);
		tmp._u = swap32(tmp._u);
		return tmp._t;
	}
	case 2:
	{
		enumCastHelper<T, u16> tmp(v);
		tmp._u = swap16(tmp._u);
		return tmp._t;
	}
	case 1:
	{
		return v;
	}
	}

	// Never reached
	return T{};
}

enum class EndianSelect
{
	Current, // Grab current endian
	// Explicitly use endian
	Big,
	Little
};
} // namespace oishii
