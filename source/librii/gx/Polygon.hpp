#pragma once

#include <array>
#include <librii/gx/Vertex.hpp>
#include <map>
#include <span>
#include <tuple>
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

inline std::pair<u32, u32> ComputeVertTriCounts(const MeshData& mesh) {
  u32 nVert = 0, nTri = 0;

  // Set of linear equations for computing polygon count from number of
  // vertices. Division by zero acts as multiplication by zero here.
  struct LinEq {
    u32 quot : 4;
    u32 dif : 4;
  };
  constexpr std::array<LinEq, (u32)PrimitiveType::Max> triVertCvt{
      LinEq{4, 0}, // 0 [v / 4] Quads
      LinEq{4, 0}, // 1 [v / 4] QuadStrip
      LinEq{3, 0}, // 2 [v / 3] Triangles
      LinEq{1, 2}, // 3 [v - 2] TriangleStrip
      LinEq{1, 2}, // 4 [v - 2] TriangleFan
      LinEq{0, 0}, // 5 [v * 0] Lines
      LinEq{0, 0}, // 6 [v * 0] LineStrip
      LinEq{0, 0}  // 7 [v * 0] Points
  };

  for (const auto& mprim : mesh.mMatrixPrimitives) {
    for (const auto& prim : mprim.mPrimitives) {
      const u8 pIdx = static_cast<u8>(prim.mType);
      assert(pIdx < static_cast<u8>(PrimitiveType::Max));
      const LinEq& pCvtr = triVertCvt[pIdx];
      const u32 pVCount = prim.mVertices.size();

      if (pVCount == 0)
        continue;

      nVert += pVCount;
      nTri += pCvtr.quot == 0 ? 0 : pVCount / pCvtr.quot - pCvtr.dif;
    }
  }

  return {nVert, nTri};
}

inline std::pair<u32, u32> computeVertTriCounts(const auto& meshes) {
  u32 nVert = 0, nTri = 0;

  for (const auto& mesh : meshes) {
    const auto [vert, tri] = librii::gx::ComputeVertTriCounts(mesh);
    nVert += vert;
    nTri += tri;
  }

  return {nVert, nTri};
}

//
// Build display matrix index (subset of mDrawMatrices)
//
inline std::set<s16> computeDisplayMatricesSubset(const auto& meshes,
                                                  const auto& bones) {
  std::set<s16> displayMatrices;
  for (const auto& mesh : meshes) {
    // TODO: Do we need to check currentMatrixEmbedded flag?
    if (mesh.mCurrentMatrix != -1) {
      displayMatrices.insert(mesh.mCurrentMatrix);
      continue;
    }
    // TODO: Presumably mCurrentMatrix (envelope mode) precludes blended weight
    // mode?
    for (auto& mp : mesh.mMatrixPrimitives) {
      for (auto& w : mp.mDrawMatrixIndices) {
        if (w == -1) {
          continue;
        }
        displayMatrices.insert(w);
      }
    }
  }
  for (const auto& bone : bones) {
    // This is ignored?
    if (bone.isDisplayMatrix() || true) {
      displayMatrices.insert(bone.matrixId);
    }
  }
  return displayMatrices;
}

} // namespace librii::gx
