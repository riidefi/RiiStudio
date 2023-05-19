#pragma once

#include "types.hxx"
#include <cstdio>
#include <iostream>
#include <rsl/DebugBreak.hpp>
#include <set>
#include <span>
#include <vector>

namespace oishii {

enum class Whence {
  Set,     // Absolute -- start of file
  Current, // Relative -- current position
  End,     // Relative -- end position
};

struct ErrorHandler {
  virtual ~ErrorHandler() = default;

  // Some care may need to be taken if multiple streams exist in parallel
  // For this reason, the stream is always passed.
  // Note: Streams will never nest errors

  virtual void onErrorBegin() = 0;
  virtual void onErrorDescribe(const char* type, const char* brief,
                               const char* details) = 0;
  // First call for the root
  virtual void onErrorAddStackTrace(std::streampos start, std::streamsize size,
                                    const char* domain) = 0;
  virtual void onErrorEnd() = 0;
};

using FlushFileHandler = void (*)(std::span<const u8> buf,
                                  std::string_view path);
void SetGlobalFileWriteFunction(FlushFileHandler handler);
void FlushFile(std::span<const u8> buf, std::string_view path);

} // namespace oishii
