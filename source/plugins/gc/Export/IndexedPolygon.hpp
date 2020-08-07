#pragma once

#include <core/3d/i3dmodel.hpp>
#include <core/common.h>
#include <plugins/gc/GX/VertexTypes.hpp>

#include "IndexedPrimitive.hpp"
#include "VertexDescriptor.hpp"

namespace libcube {
struct MatrixPrimitive {
  // Part of the polygon in G3D
  // Not the most robust solution, but currently each expoerter will pick which
  // of the data to use
  s16 mCurrentMatrix = -1;

  std::vector<s16> mDrawMatrixIndices;

  std::vector<libcube::IndexedPrimitive> mPrimitives;

  MatrixPrimitive() = default;
  MatrixPrimitive(s16 current_matrix, std::vector<s16> drawMatrixIndices)
      : mCurrentMatrix(current_matrix), mDrawMatrixIndices(drawMatrixIndices) {}
  bool operator==(const MatrixPrimitive& rhs) const {
    return mCurrentMatrix == rhs.mCurrentMatrix &&
           mDrawMatrixIndices == rhs.mDrawMatrixIndices &&
           mPrimitives == rhs.mPrimitives;
  }
};
struct MeshData {
  std::vector<MatrixPrimitive> mMatrixPrimitives;
  libcube::VertexDescriptor mVertexDescriptor;
};

struct IndexedPolygon : public riistudio::lib3d::Polygon {
  virtual void setId(u32 id) = 0;
  // // PX_TYPE_INFO("GameCube Polygon", "gc_indexedpoly",
  // "GC::IndexedPolygon"); In wii/gc, absolute indices across mprims

  virtual MeshData& getMeshData() = 0;
  virtual const MeshData& getMeshData() const = 0;
  u64 getNumPrimitives() const;
  // Triangles
  // We add this to the last mprim. May need to be split up later.
  s64 addPrimitive();
  bool hasAttrib(SimpleAttrib attrib) const override;
  void setAttrib(SimpleAttrib attrib, bool v) override;
  IndexedPrimitive* getIndexedPrimitiveFromSuperIndex(u64 idx);
  const IndexedPrimitive* getIndexedPrimitiveFromSuperIndex(u64 idx) const;
  u64 getPrimitiveVertexCount(u64 index) const;
  void resizePrimitiveVertexArray(u64 index, u64 size);
  SimpleVertex getPrimitiveVertex(u64 prim_idx, u64 vtx_idx);
  void propogate(VBOBuilder& out) const override;
  virtual glm::vec3 getPos(u64 id) const = 0;
  virtual glm::vec3 getNrm(u64 id) const = 0;
  virtual glm::vec4 getClr(u64 chan, u64 id) const = 0;
  virtual glm::vec2 getUv(u64 chan, u64 id) const = 0;

  virtual u64 addPos(const glm::vec3& v) = 0;
  virtual u64 addNrm(const glm::vec3& v) = 0;
  virtual u64 addClr(u64 chan, const glm::vec4& v) = 0;
  virtual u64 addUv(u64 chan, const glm::vec2& v) = 0;
  // We add on to the attached buffer
  void setPrimitiveVertex(u64 prim_idx, u64 vtx_idx, const SimpleVertex& vtx) {}
  void update() override {
    // Split up added primitives if necessary
  }

  // Matrix list access
  virtual u64 getMatrixPrimitiveNumIndexedPrimitive(u64 idx) const = 0;
  virtual const IndexedPrimitive&
  getMatrixPrimitiveIndexedPrimitive(u64 idx, u64 prim_idx) const = 0;
  virtual IndexedPrimitive&
  getMatrixPrimitiveIndexedPrimitive(u64 idx, u64 prim_idx) = 0;

  virtual VertexDescriptor& getVcd() { return getMeshData().mVertexDescriptor; }
  virtual const VertexDescriptor& getVcd() const {
    return getMeshData().mVertexDescriptor;
  }

  virtual std::vector<glm::mat4> getPosMtx(u64 mpId) const { return {}; }

  virtual void init(bool skinned, riistudio::lib3d::AABB* boundingBox) = 0;
  virtual void initBufsFromVcd() {}
};

} // namespace libcube
