#pragma once

#include <core/common.h>
#include <map>
#include <memory>
#include <tuple>
#include <vector>

//----------------------------------
// Vertex attribute generation
struct VAOEntry {
  u32 binding_point;
  const char* name;

  u32 format; // (gl)
  u32 size;   // (of element / stride)
};

// WIP..
struct VBOBuilder {
  VBOBuilder();
  ~VBOBuilder();

  std::vector<u8> mData;
  std::vector<u32> mIndices;
  struct SplicePoint {
    std::size_t offset = 0;
    std::size_t size = (std::size_t)-1;
  };

  // binding_point : data
  std::map<u32, std::pair<VAOEntry, std::vector<u8>>> mPropogating;

  int getNumSplices() { return splicePoints.size() - 1; }
  SplicePoint getSplice(std::size_t i) { return splicePoints[i]; }
  std::vector<SplicePoint> getSplicesInRange(std::size_t start,
                                             std::size_t ofs);

  void build();
  void markSplice() {
    SplicePoint pt{mIndices.size()};
    splicePoints[splicePoints.size() - 1].size =
        mIndices.size() - splicePoints[splicePoints.size() - 1].offset;

    splicePoints.push_back(pt);
  }

  template <typename T> void pushData(u32 binding_point, const T& data) {
    auto& attrib_buf = mPropogating[binding_point];
    const std::size_t begin = attrib_buf.second.size();
    attrib_buf.second.resize(attrib_buf.second.size() + sizeof(T));
    *reinterpret_cast<T*>(attrib_buf.second.data() + begin) = data;
  }
  u32 VAO;
  u32 mPositionBuf, mIndexBuf;

private:
  // Splice point end markers. first always starts at 0
  std::vector<SplicePoint> splicePoints;
  template <typename T> void push(const T& data) {
    const std::size_t begin = mData.size();
    mData.resize(mData.size() + sizeof(T));
    *reinterpret_cast<T*>(mData.data() + begin) = data;
  }
};
