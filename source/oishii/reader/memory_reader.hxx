#pragma once

#include "../interfaces.hxx"

#include <array> // Why?
#include <memory>
#include <vector>

namespace oishii {
class MemoryBlockReader : public IReader {
public:
  MemoryBlockReader(u32 sz)
      : mPos(0), mBufSize(sz), mBuf(std::vector<u8>(sz)) {}
  MemoryBlockReader(std::vector<u8> buf, u32 sz)
      : mPos(0), mBufSize(sz), mBuf(std::move(buf)) {}
  virtual ~MemoryBlockReader() {}

  u32 tell() final override { return mPos; }
  void seekSet(u32 ofs) final override { mPos = ofs; }
  u32 startpos() final override { return 0; }
  u32 endpos() final override { return mBufSize; }
  u8* getStreamStart() final override { return mBuf.data(); }

  // Faster bound check
  inline bool isInBounds(u32 pos) {
    // Can't be negative, start always at zero
    return pos < mBufSize;
  }

private:
  u32 mPos;
  u32 mBufSize;
  std::vector<u8> mBuf;
};

} // namespace oishii
