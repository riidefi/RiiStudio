#pragma once

#include "../interfaces.hxx"
#include <memory>
#include <vector>

namespace oishii {

//! @brief Writer with expanding buffer.
//!
class VectorWriter : public AbstractStream<VectorWriter> {
public:
  VectorWriter(u32 buffer_size) : mPos(0), mBuf(buffer_size) {}
  VectorWriter(std::vector<u8> buf) : mPos(0), mBuf(std::move(buf)) {}
  virtual ~VectorWriter() = default;

  u32 tell() { return mPos; }
  void seekSet(u32 ofs) { mPos = ofs; }
  u32 startpos() { return 0; }
  u32 endpos() { return (u32)mBuf.size(); }

  // Bound check unlike reader -- can always extend file
  inline bool isInBounds(u32 pos) { return pos < mBuf.size(); }

  void attachDataForMatchingOutput(const std::vector<u8>& data) {
#ifndef NDEBUG
    mDebugMatch = data;
#endif
  }

protected:
  u32 mPos;
  std::vector<u8> mBuf;
#ifndef NDEBUG
  std::vector<u8> mDebugMatch;
#endif
public:
  void resize(u32 sz) { mBuf.resize(sz); }
  u8* getDataBlockStart() { return mBuf.data(); }
  u32 getBufSize() { return (u32)mBuf.size(); }
};

} // namespace oishii
