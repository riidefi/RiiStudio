/*!
 * @file
 * @brief Utilities for endian streams.
 */

#pragma once

#include <bit>
static_assert(__cpp_lib_byteswap >= 202110L, "Depends on std::byteswap");

#include <oishii/options.hxx>
#include <stdint.h>

#include <core/common.h>

namespace oishii {
// Console colors

#ifdef _MSC_VER
#define IF_WIN(x) x
#include <Windows.h>
#else
#define IF_WIN(x)
#endif

struct Console {
  IF_WIN(HANDLE handle);
  IF_WIN(CONSOLE_SCREEN_BUFFER_INFO prior_state);
  IF_WIN(uint16_t last); // last attribute

  void restoreFirstColorState() {
    IF_WIN(SetConsoleTextAttribute(handle, prior_state.wAttributes));
  }

  // Note: last is a single value, not a stack -- cannot nest
  // for this reason, deprecated

  //	void restoreLastColorState()
  //	{
  //		setColorState(last);
  //	}
  void setColorState(uint16_t attrib) {
    IF_WIN(last = attrib);
    IF_WIN(SetConsoleTextAttribute(handle, attrib));
  }

#ifdef _MSC_VER
  Console() : handle(GetStdHandle(STD_OUTPUT_HANDLE)) {
    GetConsoleScreenBufferInfo(handle, &prior_state);
    last = prior_state.wAttributes;
  }
  ~Console() { restoreFirstColorState(); }
#endif
  static Console sInstance;
  static Console& getInstance() { return sInstance; }
};
#ifndef _MSC_VER
struct ScopedFormatter {
  ScopedFormatter(uint16_t) {}
};
#else
struct ScopedFormatter {
  uint16_t last;

  ScopedFormatter(uint16_t attrib) {
    last = Console::getInstance().last;
    Console::getInstance().setColorState(attrib);
  }
  ~ScopedFormatter() { Console::getInstance().setColorState(last); }
};
#endif

template <typename T1, typename T2> union enumCastHelper {
  // Switch above means this always is instantiated
  // static_assert(sizeof(T1) == sizeof(T2), "Sizes of types must match.");
  T1 _t;
  T2 _u;

  enumCastHelper(T1 _) : _t(_) {}
};

#if OISHII_PLATFORM_LE == 1
#define MAKE_BE32(x) std::byteswap(x)
#define MAKE_LE32(x) x
#else
#define MAKE_BE32(x) x
#define MAKE_LE32(x) std::byteswap(x)
#endif

//! @brief Fast endian swapping.
//!
//! @tparam T Type of value to swap. Must be sized 1, 2, or 4
//!
//! @return T endian swapped.
template <typename T> inline T swapEndian(T v) {
  static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4,
                "T must of size 1, 2, or 4");

  switch (sizeof(T)) {
  case 4: {
    enumCastHelper<T, uint32_t> tmp(v);
    tmp._u = std::byteswap(tmp._u);
    return tmp._t;
  }
  case 2: {
    enumCastHelper<T, uint16_t> tmp(v);
    tmp._u = std::byteswap(tmp._u);
    return tmp._t;
  }
  case 1: {
    return v;
  }
  }

  // Never reached
  return T{};
}

enum class EndianSelect {
  Current, // Grab current endian
  // Explicitly use endian
  Big,
  Little
};

template <uint32_t size> struct integral_of_equal_size;

template <> struct integral_of_equal_size<1> {
  using type = uint8_t;
};

template <> struct integral_of_equal_size<2> {
  using type = uint16_t;
};

template <> struct integral_of_equal_size<4> {
  using type = uint32_t;
};

template <typename T>
using integral_of_equal_size_t =
    typename integral_of_equal_size<sizeof(T)>::type;

template <typename T, EndianSelect E = EndianSelect::Current>
inline T endianDecode(T val, std::endian fileEndian) {
  if constexpr (E == EndianSelect::Big) {
    return std::endian::native != std::endian::big ? swapEndian<T>(val) : val;
  } else if constexpr (E == EndianSelect::Little) {
    return std::endian::native != std::endian::little ? swapEndian<T>(val)
                                                      : val;
  } else if constexpr (E == EndianSelect::Current) {
    return std::endian::native != fileEndian ? swapEndian<T>(val) : val;
  }

  return val;
}

std::expected<std::vector<u8>, std::string> UtilReadFile(std::string_view path);
using FlushFileHandler = void (*)(std::span<const uint8_t> buf,
                                  std::string_view path);
void SetGlobalFileWriteFunction(FlushFileHandler handler);
void FlushFile(std::span<const uint8_t> buf, std::string_view path);

} // namespace oishii
