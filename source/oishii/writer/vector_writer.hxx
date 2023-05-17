#pragma once

#include "../VectorStream.hxx"
#include "../interfaces.hxx"
#include <memory>
#include <vector>

namespace oishii {

//! @brief Writer with expanding buffer.
//!
class VectorWriter : public VectorStream {
public:
  using VectorStream::VectorStream;

  // Bound check unlike reader -- can always extend file
  bool isInBounds(u32 pos) { return pos < mBuf.size(); }

  void attachDataForMatchingOutput(const std::vector<u8>& data) {
#ifndef NDEBUG
    mDebugMatch = data;
#endif
  }

protected:
#ifndef NDEBUG
  std::vector<u8> mDebugMatch;
#endif
};

} // namespace oishii
