#pragma once

#include "interfaces.hxx"

#include "BreakpointHolder.hxx"

namespace oishii {

class AbstractStream : public BreakpointHolder {
public:
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
