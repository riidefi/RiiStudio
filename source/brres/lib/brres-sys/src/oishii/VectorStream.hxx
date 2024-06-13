#pragma once

#include "AbstractStream.hxx"
#include <vector>

namespace oishii {

class VectorStream : public AbstractStream {
public:
  VectorStream() = default;
  VectorStream(uint32_t buffer_size) : mBuf(buffer_size) {}
  VectorStream(std::vector<uint8_t> buf) : mBuf(std::move(buf)) {}

  virtual void seekSet(uint32_t pos) override { mPos = pos; }
  virtual uint32_t tell() const override { return mPos; }
  virtual uint32_t endpos() const override { return mBuf.size(); }

  void resize(uint32_t sz) { mBuf.resize(sz); }
  uint8_t* getDataBlockStart() { return mBuf.data(); }
  const uint8_t* getStreamStart() const { return mBuf.data(); }
  uint32_t getBufSize() { return static_cast<uint32_t>(mBuf.size()); }
  std::vector<uint8_t>&& takeBuf() { return std::move(mBuf); }

  std::vector<uint8_t> mBuf;
  uint32_t mPos = 0;
};

} // namespace oishii
