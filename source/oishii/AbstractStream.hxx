#pragma once

#include "interfaces.hxx"

namespace oishii {

class BreakpointHolder {
public:
  void breakPointProcess(u32 tell, u32 size);
#ifndef NDEBUG
  struct BP {
    u32 offset, size;
    BP(u32 o, u32 s) : offset(o), size(s) {}
  };
  void add_bp(u32 offset, u32 size) { mBreakPoints.emplace_back(offset, size); }
  std::vector<BP> mBreakPoints;
#else
  void add_bp(u32, u32) {}
#endif

  template <typename U> void add_bp(u32 offset) { add_bp(offset, sizeof(U)); }
};

class AbstractStream : public BreakpointHolder {
public:
  void breakPointProcess(u32 size);

  template <Whence W = Whence::Set> void seek(int ofs, u32 mAtPool = 0) {
    static_assert(W == Whence::Set || W == Whence::Current || W == Whence::End,
                  "Invalid whence.");
    switch (W) {
    case Whence::Set:
      seekSet(ofs);
      break;
    case Whence::Current:
      if (ofs != 0) {
        seekSet(tell() + ofs);
      }
      break;
    case Whence::End:
      seekSet(endpos() - ofs);
      break;
    }
  }

  void skip(int ofs) { seek<Whence::Current>(ofs); }

  virtual void seekSet(u32 pos) = 0;
  virtual u32 tell() const = 0;
  virtual u32 endpos() const = 0;
};

} // namespace oishii
