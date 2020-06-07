#pragma once

#include "types.hxx"
#include <cstdio>
#include <set>
#include <vector>
#include <iostream>

namespace oishii {

enum class Whence {
  Set,     // Absolute -- start of file
  Current, // Relative -- current position
  End,     // Relative -- end position
  // Only valid for invalidities so far
  Last // Relative -- last read value
};

class AbstractStream;

struct ErrorHandler {
  virtual ~ErrorHandler() = default;

  // Some care may need to be taken if multiple streams exist in parallel
  // For this reason, the stream is always passed.
  // Note: Streams will never nest errors

  virtual void onErrorBegin(AbstractStream& stream) = 0;
  virtual void onErrorDescribe(AbstractStream& stream, const char* type,
                               const char* brief, const char* details) = 0;
  // First call for the root
  virtual void onErrorAddStackTrace(AbstractStream& stream,
                                    std::streampos start, std::streamsize size,
                                    const char* domain) = 0;
  virtual void onErrorEnd(AbstractStream& stream) = 0;
};

class AbstractStream {
public:
  virtual u32 tell() = 0;
  virtual void seekSet(u32 ofs) = 0;
  virtual u32 startpos() = 0;
  virtual u32 endpos() = 0;

  void breakPointProcess(u32 size) {
#ifndef NDEBUG
    for (const auto& bp : mBreakPoints) {
      if (tell() >= bp.offset && tell() + size <= bp.offset + bp.size) {
        printf("Writing to %04u (0x%04x) sized %u\n", tell(), tell(), size);
        // warnAt("Breakpoint hit", tell(), tell() + sizeof(T));
        __debugbreak();
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
  template <typename T> void add_bp(u32 offset) { add_bp(offset, sizeof(T)); }
  std::vector<BP> mBreakPoints;
#else
  void add_bp(u32, u32) {}
  template <typename T> void add_bp(u32) {}
#endif

  // Faster version for memoryblock
  template <Whence W = Whence::Last>
  inline void seek(int ofs, u32 mAtPool = 0) {
    static_assert(W != Whence::Last, "Cannot use last seek yet.");
    static_assert(W == Whence::Set || W == Whence::Current || W == Whence::End,
                  "Invalid whence.");
    switch (W) {
    case Whence::Set:
      seekSet(ofs);
      break;
    case Whence::Current:
      if (ofs != 0)
        seekSet(tell() + ofs);
      break;
    case Whence::End:
      seekSet(endpos() - ofs);
      break;
    }
  }

  inline void skip(int ofs) { seek<Whence::Current>(ofs); }

  void addErrorHandler(ErrorHandler* handler) {
    mErrorHandlers.emplace(handler);
  }
  void removeErrorHandler(ErrorHandler* handler) {
    mErrorHandlers.erase(handler);
  }

protected:
  void beginError() {
    for (auto* handler : mErrorHandlers)
      handler->onErrorBegin(*this);
  }
  void describeError(const char* type, const char* brief, const char* details) {
    for (auto* handler : mErrorHandlers)
      handler->onErrorDescribe(*this, type, brief, details);
  }
  void addErrorStackTrace(std::streampos start, std::streamsize size,
                          const char* domain) {
    for (auto* handler : mErrorHandlers)
      handler->onErrorAddStackTrace(*this, start, size, domain);
  }
  void endError() {
    for (auto* handler : mErrorHandlers)
      handler->onErrorEnd(*this);
  }

private:
  std::set<ErrorHandler*> mErrorHandlers;
};

class IReader : public AbstractStream {
public:
  virtual unsigned char* getStreamStart() = 0;
};

class IWriter : public AbstractStream {};
} // namespace oishii
