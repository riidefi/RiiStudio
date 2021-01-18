#pragma once

#include <array>
#include <librii/gx/Vertex.hpp>
#include <map>
#include <vector>

namespace librii::gx {

struct VertexDescriptor {
  std::map<VertexAttribute, VertexAttributeType> mAttributes;
  u32 mBitfield; // values of VertexDescriptor

  u64 getVcdSize() const {
    u64 vcd_size = 0;
    for (s64 i = 0; i < (s64)VertexAttribute::Max; ++i)
      if (mBitfield & (1 << 1))
        ++vcd_size;
    return vcd_size;
  }
  void calcVertexDescriptorFromAttributeList() {
    mBitfield = 0;
    for (VertexAttribute i = VertexAttribute::PositionNormalMatrixIndex;
         static_cast<u64>(i) < static_cast<u64>(VertexAttribute::Max);
         i = static_cast<VertexAttribute>(static_cast<u64>(i) + 1)) {
      auto found = mAttributes.find(i);
      if (found != mAttributes.end() &&
          found->second != VertexAttributeType::None)
        mBitfield |= (1 << static_cast<u64>(i));
    }
  }
  bool operator[](VertexAttribute attr) const {
    return mBitfield & (1 << static_cast<u64>(attr));
  }
  bool operator==(const VertexDescriptor& rhs) const {
    // FIXME: remove NULL attribs before comparing
    return mAttributes.size() == rhs.mAttributes.size() &&
           std::equal(mAttributes.begin(), mAttributes.end(),
                      rhs.mAttributes.begin());
  }
};

struct IndexedVertex {
  const u16& operator[](VertexAttribute attr) const {
    assert((u64)attr < (u64)VertexAttribute::Max);
    return indices[(u64)attr];
  }
  u16& operator[](VertexAttribute attr) {
    assert((u64)attr < (u64)VertexAttribute::Max);
    return indices[(u64)attr];
  }
  bool operator==(const IndexedVertex& rhs) const = default;

private:
  std::array<u16, (u64)VertexAttribute::Max> indices;
};

struct IndexedPrimitive {
  PrimitiveType mType;
  std::vector<IndexedVertex> mVertices;

  IndexedPrimitive() = default;
  IndexedPrimitive(PrimitiveType type, u64 size)
      : mType(type), mVertices(size) {}
  bool operator==(const IndexedPrimitive& rhs) const = default;
};

struct MatrixPrimitive {
  // Part of the polygon in G3D
  // Not the most robust solution, but currently each expoerter will pick which
  // of the data to use
  s16 mCurrentMatrix = -1;

  std::vector<s16> mDrawMatrixIndices; // TODO: Fixed size array

  std::vector<IndexedPrimitive> mPrimitives;

  MatrixPrimitive() = default;
  MatrixPrimitive(s16 current_matrix, std::vector<s16> drawMatrixIndices)
      : mCurrentMatrix(current_matrix), mDrawMatrixIndices(drawMatrixIndices) {}
  bool operator==(const MatrixPrimitive& rhs) const = default;
};

struct MeshData {
  std::vector<MatrixPrimitive> mMatrixPrimitives;
  VertexDescriptor mVertexDescriptor;

  bool operator==(const MeshData&) const = default;
};

} // namespace librii::gx
