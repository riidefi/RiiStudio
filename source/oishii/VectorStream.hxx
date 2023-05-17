#pragma once

#include "AbstractStream.hxx"
#include <vector>

namespace oishii {

class VectorStream : public AbstractStream {
public:
  VectorStream() = default;
  VectorStream(u32 buffer_size) : mBuf(buffer_size) {}
  VectorStream(std::vector<u8> buf) : mBuf(std::move(buf)) {}

  virtual void seekSet(u32 pos) override { mPos = pos; }
  virtual u32 tell() const override { return mPos; }
  virtual u32 endpos() const { return mBuf.size(); }

  void resize(u32 sz) { mBuf.resize(sz); }
  u8* getDataBlockStart() { return mBuf.data(); }
  const u8* getStreamStart() const { return mBuf.data(); }
  u32 getBufSize() { return static_cast<u32>(mBuf.size()); }
  std::vector<u8>&& takeBuf() { return std::move(mBuf); }

  std::vector<u8> mBuf;
  u32 mPos = 0;
};

} // namespace oishii
