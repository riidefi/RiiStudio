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

struct _CONSOLE_SCREEN_BUFFER_INFO;

namespace oishii {
// Console colors

#ifdef _MSC_VER
#define IF_WIN(x) x
#else
#define IF_WIN(x)
#endif

struct Console {
  IF_WIN(void* handle);
  IF_WIN(std::unique_ptr<_CONSOLE_SCREEN_BUFFER_INFO> prior_state);
  IF_WIN(uint16_t last);

  void restoreFirstColorState();
  void setColorState(uint16_t attrib);

  static Console sInstance;
  static Console& getInstance();

#ifdef _MSC_VER
  Console();
  ~Console();
#endif
};

#ifndef _MSC_VER
struct ScopedFormatter {
  ScopedFormatter(uint16_t) {}
};
#else
struct ScopedFormatter {
  uint16_t last;

  ScopedFormatter(uint16_t attrib);
  ~ScopedFormatter();
};
#endif

std::expected<std::vector<u8>, std::string> UtilReadFile(std::string_view path);
using FlushFileHandler = void (*)(std::span<const uint8_t> buf,
                                  std::string_view path);
void SetGlobalFileWriteFunction(FlushFileHandler handler);
void FlushFile(std::span<const uint8_t> buf, std::string_view path);

} // namespace oishii
