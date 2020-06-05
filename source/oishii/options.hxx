#pragma once

#ifdef __cplusplus
namespace oishii {
#endif
	// TODO: Use OISHII_DEBUG and OISHII_RELEASE not poular DEBUG/RELEASE
	// TODO: Implement bounds checking and enable by default on debug builds

#if defined(_DEBUG) && !defined(DEBUG)
#define DEBUG
#endif

#ifdef _NDEBUG
#ifndef RELEASE
#define RELEASE
#endif
#ifdef DEBUG
#undef DEBUG
#endif
#endif

#if defined(DEBUG) && defined(RELEASE)
#pragma error "Cannot build for debug and release."
#endif


/*
Options:
	OISHII_MULTIENDIAN_SUPPORT	(Default: Enabled)
	OISHII_PLATFORM_LE			(Default: Enabled)
	OISHII_BOUNDS_CHECK			(Default: Disabled -- not implemented fully)
	OISHII_ALIGNMENT_CHECK		(Default: Disabled on release, enabled on debug)
*/


// Prefer options enumeration to macros
	enum Options
	{
#ifdef DEBUG
		IF_DEBUG = 1,
#define OISHII_IF_DEBUG 1
#else
		IF_DEBUG = 0,
#define OISHII_IF_DEBUG = 0
#endif

#ifdef OISHII_MULTIENDIAN_SUPPORT
		MULTIENDIAN_SUPPORT = OISHII_MULTIENDIAN_SUPPORT,
#else
		MULTIENDIAN_SUPPORT = 1,
#define OISHII_MULTIENDIAN_SUPPORT 1
#endif

#ifdef OISHII_PLATFORM_LE
		PLATFORM_LE = OISHII_PLATFORM_LE,
#else
		PLATFORM_LE = 1,
#define OISHII_PLATFORM_LE 1
#endif

#ifdef OISHII_BOUNDS_CHECK
		BOUNDS_CHECK = OISHII_BOUNDS_CHECK
#else
		BOUNDS_CHECK = IF_DEBUG,
#define OISHII_BOUNDS_CHECK OISHII_IF_DEBUG
#endif

#ifdef OISHII_ALIGNMENT_CHECK
		ALIGNMENT_CHECK = OISHII_ALIGNMENT_CHECK,
#else
		ALIGNMENT_CHECK = IF_DEBUG
#define OISHII_ALIGNMENT_CHECK OISHII_IF_DEBUG
#endif
	};

#ifdef __cplusplus
} // namespace oishii
#endif
