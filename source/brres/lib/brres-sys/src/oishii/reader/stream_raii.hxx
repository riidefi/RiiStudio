#pragma once

#include "binary_reader.hxx"

namespace oishii {

template <Whence W = Whence::Set, typename T = BinaryReader> struct Jump {
  inline Jump(T& stream, uint32_t offset)
      : mStream(stream), back(stream.tell()) {
    mStream.template seek<W>(offset);
  }
  inline ~Jump() { mStream.template seek<Whence::Set>(back); }
  T& mStream;
  uint32_t back;
};

template <Whence W = Whence::Current, typename T = BinaryReader>
struct JumpOut {
  inline JumpOut(T& stream, uint32_t offset)
      : mStream(stream), start(stream.tell()), back(offset) {}
  inline ~JumpOut() {
    mStream.template seek<Whence::Set>(start);
    mStream.template seek<W>(back);
  }
  T& mStream;

  uint32_t start;
  uint32_t back;
};

template <typename T = BinaryReader>
struct DebugExpectSized
#ifdef RELEASE
{
  DebugExpectSized(T& stream, uint32_t size) {}
  bool assertSince(uint32_t) { return true; }
};
#else
{
  inline DebugExpectSized(T& stream, uint32_t size)
      : mStream(stream), mStart(stream.tell()), mSize(size) {}
  inline ~DebugExpectSized() {
    if (mStream.tell() - mStart != mSize)
      printf("Expected to read %u bytes -- instead read %u\n", mSize,
             mStream.tell() - mStart);
    // assert(mStream.tell() - mStart == mSize && "Invalid size for this scope!");
  }

  bool assertSince(uint32_t dif) {
    const auto real_dif = mStream.tell() - mStart;

    if (real_dif != dif) {
      printf("Needed to read %u (%x) bytes; instead read %u (%x)\n", dif, dif,
             real_dif, real_dif);
      return false;
    }
    return true;
  }

  T& mStream;
  uint32_t mStart;
  uint32_t mSize;
};
#endif

} // namespace oishii
