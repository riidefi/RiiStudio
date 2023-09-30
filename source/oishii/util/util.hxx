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


std::expected<std::vector<u8>, std::string> UtilReadFile(std::string_view path);
using FlushFileHandler = void (*)(std::span<const uint8_t> buf,
                                  std::string_view path);
void SetGlobalFileWriteFunction(FlushFileHandler handler);
void FlushFile(std::span<const uint8_t> buf, std::string_view path);

} // namespace oishii
