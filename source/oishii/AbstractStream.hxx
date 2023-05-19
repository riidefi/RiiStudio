#pragma once

#include "interfaces.hxx"

namespace oishii {

class BreakpointHolder {
public:
  void breakPointProcess(uint32_t tell, uint32_t size);
#ifndef NDEBUG
  struct BP {
    uint32_t offset, size;
    BP(uint32_t o, uint32_t s) : offset(o), size(s) {}
  };
  void add_bp(uint32_t offset, uint32_t size) {
    mBreakPoints.emplace_back(offset, size);
  }
  std::vector<BP> mBreakPoints;
#else
  void add_bp(uint32_t, uint32_t) {}
#endif

  template <typename U> void add_bp(uint32_t offset) {
    add_bp(offset, sizeof(U));
  }
};

class AbstractStream : public BreakpointHolder {
public:
  void breakPointProcess(uint32_t size);

  template <Whence W = Whence::Set> void seek(int ofs, uint32_t mAtPool = 0) {
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

  virtual void seekSet(uint32_t pos) = 0;
  virtual uint32_t tell() const = 0;
  virtual uint32_t endpos() const = 0;
};

} // namespace oishii
