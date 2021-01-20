#pragma once

#include "data_provider.hxx"
#include "types.hxx"
#include <cstdio>
#include <iostream>
#include <set>
#include <vector>

namespace oishii {

enum class Whence {
  Set,     // Absolute -- start of file
  Current, // Relative -- current position
  End,     // Relative -- end position
  // Only valid for invalidities so far
  Last // Relative -- last read value
};

struct ErrorHandler {
  virtual ~ErrorHandler() = default;

  // Some care may need to be taken if multiple streams exist in parallel
  // For this reason, the stream is always passed.
  // Note: Streams will never nest errors

  virtual void onErrorBegin(const DataProvider& stream) = 0;
  virtual void onErrorDescribe(const DataProvider& stream, const char* type,
                               const char* brief, const char* details) = 0;
  // First call for the root
  virtual void onErrorAddStackTrace(const DataProvider& stream,
                                    std::streampos start, std::streamsize size,
                                    const char* domain) = 0;
  virtual void onErrorEnd(const DataProvider& stream) = 0;
};

template <typename T> class AbstractStream {
private:
  T& derived() { return *static_cast<T*>(this); }

public:
  void breakPointProcess(u32 size) {
#ifndef NDEBUG
    for (const auto& bp : mBreakPoints) {
      if (derived().tell() >= bp.offset &&
          derived().tell() + size <= bp.offset + bp.size) {
        printf("Writing to %04u (0x%04x) sized %u\n", derived().tell(),
               derived().tell(), size);
        // warnAt("Breakpoint hit", tell(), tell() + sizeof(T));
#ifdef _MSC_VER
        __debugbreak();
#endif
      }
    }
#endif
  }

#ifndef NDEBUG
public:
  struct BP {
    u32 offset, size;
    BP(u32 o, u32 s) : offset(o), size(s) {}
  };
  void add_bp(u32 offset, u32 size) { mBreakPoints.emplace_back(offset, size); }
  template <typename U> void add_bp(u32 offset) { add_bp(offset, sizeof(U)); }
  std::vector<BP> mBreakPoints;
#else
  void add_bp(u32, u32) {}
  template <typename U> void add_bp(u32) {}
#endif

  // Faster version for memoryblock
  template <Whence W = Whence::Last>
  inline void seek(int ofs, u32 mAtPool = 0) {
    static_assert(W != Whence::Last, "Cannot use last seek yet.");
    static_assert(W == Whence::Set || W == Whence::Current || W == Whence::End,
                  "Invalid whence.");
    switch (W) {
    case Whence::Set:
      derived().seekSet(ofs);
      break;
    case Whence::Current:
      if (ofs != 0)
        derived().seekSet(derived().tell() + ofs);
      break;
    case Whence::End:
      derived().seekSet(derived().endpos() - ofs);
      break;
    }
    // assert(derived().tell() < derived().endpos());
  }

  inline void skip(int ofs) { seek<Whence::Current>(ofs); }
};

} // namespace oishii
