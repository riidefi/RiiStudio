#pragma once

#include <array>
#include <core/common.h>
#include <librii/gx.h>
#include <string>

namespace librii::g3d {

struct PolygonData : public librii::gx::MeshData {
  std::string mName;

  s16 mCurrentMatrix = -1; // Part of the polygon in G3D

  bool visible = true;

  // For IDs, set to -1 in binary to not exist. Here, empty string
  std::string mPositionBuffer;
  std::string mNormalBuffer;
  std::array<std::string, 2> mColorBuffer;
  std::array<std::string, 8> mTexCoordBuffer;

  std::string getName() const { return mName; }

  // SYNC with BinaryPolygon
  bool needsPositionMtx() const {
    // TODO: Do we need to check currentMatrixEmbedded?
    return mCurrentMatrix == -1;
  }
  bool needsNormalMtx() const {
    // TODO: Do we need to check currentMatrixEmbedded?
    return mVertexDescriptor[gx::VertexAttribute::Normal];
  }
  bool needsAnyTextureMtx() const {
    // TODO: Do we need to check currentMatrixEmbedded?
    u32 tex0idx = static_cast<u32>(gx::VertexAttribute::Texture0MatrixIndex);
    return mVertexDescriptor.mBitfield & (0b1111'1111 << tex0idx);
  }
  bool needsTextureMtx(u32 i) const {
    // TODO: Do we need to check currentMatrixEmbedded?
    assert(i < 8);
    u32 tex0idx = static_cast<u32>(gx::VertexAttribute::Texture0MatrixIndex);
    return mVertexDescriptor.mBitfield & (1 << (tex0idx + i));
  }

  bool operator==(const PolygonData& rhs) const = default;
};

} // namespace librii::g3d
