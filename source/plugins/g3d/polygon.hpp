#pragma once

#include <core/common.h>
#include <plugins/gc/Export/IndexedPolygon.hpp>
#include <librii/gx.h>
#include <vector>

#include <core/kpi/Node.hpp>

namespace riistudio::g3d {

class Model;

using MatrixPrimitive = libcube::MatrixPrimitive;
struct PolygonData : public libcube::MeshData {
  std::string mName;
  u32 mId;

  s16 mCurrentMatrix = -1; // Part of the polygon in G3D

  bool currentMatrixEmbedded = false;
  bool visible = true;

  // For IDs, set to -1 in binary to not exist. Here, empty string
  std::string mPositionBuffer;
  std::string mNormalBuffer;
  std::array<std::string, 2> mColorBuffer;
  std::array<std::string, 8> mTexCoordBuffer;

  bool operator==(const PolygonData& rhs) const {
    return mName == rhs.mName && mId == rhs.mId &&
           mCurrentMatrix == rhs.mCurrentMatrix &&
           currentMatrixEmbedded == rhs.currentMatrixEmbedded &&
           visible == rhs.visible && mPositionBuffer == rhs.mPositionBuffer &&
           mNormalBuffer == rhs.mNormalBuffer &&
           mColorBuffer == rhs.mColorBuffer &&
           mTexCoordBuffer == rhs.mTexCoordBuffer &&
           mVertexDescriptor == rhs.mVertexDescriptor &&
           mMatrixPrimitives == rhs.mMatrixPrimitives;
  }
};
struct Polygon : public PolygonData,
                 public libcube::IndexedPolygon,
                 public virtual kpi::IObject {
  void setId(u32 id) override { mId = id; }
  virtual const g3d::Model* getParent() const;
  g3d::Model* getMutParent();
  std::string getName() const { return mName; }
  void setName(const std::string& name) override { mName = name; }

  // Matrix list access
  u64 getMatrixPrimitiveNumIndexedPrimitive(u64 idx) const override {
    assert(idx < mMatrixPrimitives.size());
    if (idx < mMatrixPrimitives.size())
      return mMatrixPrimitives[idx].mPrimitives.size();
    return 0;
  }
  const libcube::IndexedPrimitive&
  getMatrixPrimitiveIndexedPrimitive(u64 idx, u64 prim_idx) const override {
    assert(idx < mMatrixPrimitives.size());
    assert(prim_idx < mMatrixPrimitives[idx].mPrimitives.size());
    return mMatrixPrimitives[idx].mPrimitives[prim_idx];
  }
  libcube::IndexedPrimitive&
  getMatrixPrimitiveIndexedPrimitive(u64 idx, u64 prim_idx) override {
    assert(idx < mMatrixPrimitives.size());
    assert(prim_idx < mMatrixPrimitives[idx].mPrimitives.size());
    return mMatrixPrimitives[idx].mPrimitives[prim_idx];
  }
  MeshData& getMeshData() override { return *this; }
  const MeshData& getMeshData() const { return *this; }
  lib3d::AABB getBounds() const override {
    // TODO
    return {{0, 0, 0}, {0, 0, 0}};
  }
  std::vector<glm::mat4> getPosMtx(u64 mpId) const override;

  glm::vec2 getUv(u64 chan, u64 id) const override;
  glm::vec4 getClr(u64 chan, u64 id) const override;
  glm::vec3 getPos(u64 id) const override;
  glm::vec3 getNrm(u64 id) const override;
  u64 addPos(const glm::vec3& v) override;
  u64 addNrm(const glm::vec3& v) override;
  u64 addClr(u64 chan, const glm::vec4& v) override;
  u64 addUv(u64 chan, const glm::vec2& v) override;

  void addTriangle(std::array<SimpleVertex, 3> tri) override;
  void init(bool skinned, riistudio::lib3d::AABB* boundingBox) override {
    // TODO: Handle skinning...
    (void)skinned;
    // No bounding box here
    (void)boundingBox;
  }
  void initBufsFromVcd() override;
  bool operator==(const Polygon& rhs) const {
    return PolygonData::operator==(rhs);
  }
};

} // namespace riistudio::g3d
