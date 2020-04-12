#pragma once

#include <core/common.h>
#include <plugins/gc/Export/IndexedPolygon.hpp>
#include <plugins/gc/GX/VertexTypes.hpp>
#include <vector>

namespace riistudio::j3d {

struct Model;

struct MatrixPrimitive {
  s16 mCurrentMatrix = -1; // Part of the polygon in G3D

  std::vector<s16> mDrawMatrixIndices;

  std::vector<libcube::IndexedPrimitive> mPrimitives;

  MatrixPrimitive() = default;
  MatrixPrimitive(s16 current_matrix, std::vector<s16> drawMatrixIndices)
      : mCurrentMatrix(current_matrix), mDrawMatrixIndices(drawMatrixIndices) {}
  bool operator==(const MatrixPrimitive &rhs) const {
    return mCurrentMatrix == rhs.mCurrentMatrix &&
           mDrawMatrixIndices == rhs.mDrawMatrixIndices &&
           mPrimitives == rhs.mPrimitives;
  }
};
struct ShapeData {
  u32 id;
  enum class Mode {
    Normal,
    Billboard_XY,
    Billboard_Y,
    Skinned,

    Max
  };
  Mode mode = Mode::Normal;

  f32 bsphere = 100000.0f;
  lib3d::AABB bbox{{-100000.0f, -100000.0f, -100000.0f},
                   {100000.0f, 100000.0f, 100000.0f}};

  std::vector<MatrixPrimitive> mMatrixPrimitives;
  libcube::VertexDescriptor mVertexDescriptor;

  bool operator==(const ShapeData &rhs) const {
    return id == rhs.id && mode == rhs.mode && bsphere == rhs.bsphere &&
           bbox == rhs.bbox && mMatrixPrimitives == rhs.mMatrixPrimitives &&
           mVertexDescriptor == rhs.mVertexDescriptor;
  }
};
struct Shape : public ShapeData, public libcube::IndexedPolygon {
  virtual const kpi::IDocumentNode *getParent() const { return nullptr; }

  std::string getName() const { return "Shape " + std::to_string(id); }
  void setName(const std::string &name) override {}
  u64 getNumMatrixPrimitives() const override {
    return mMatrixPrimitives.size();
  }
  s32 addMatrixPrimitive() override {
    mMatrixPrimitives.emplace_back();
    return (s32)mMatrixPrimitives.size() - 1;
  }
  s16 getMatrixPrimitiveCurrentMatrix(u64 idx) const override {
    assert(idx < mMatrixPrimitives.size());
    if (idx < mMatrixPrimitives.size())
      return mMatrixPrimitives[idx].mCurrentMatrix;
    else
      return -1;
  }
  void setMatrixPrimitiveCurrentMatrix(u64 idx, s16 mtx) override {
    assert(idx < mMatrixPrimitives.size());
    if (idx < mMatrixPrimitives.size())
      mMatrixPrimitives[idx].mCurrentMatrix = mtx;
  }
  // Matrix list access
  u64 getMatrixPrimitiveNumIndexedPrimitive(u64 idx) const override {
    assert(idx < mMatrixPrimitives.size());
    if (idx < mMatrixPrimitives.size())
      return mMatrixPrimitives[idx].mPrimitives.size();
    return 0;
  }
  const libcube::IndexedPrimitive &
  getMatrixPrimitiveIndexedPrimitive(u64 idx, u64 prim_idx) const override {
    assert(idx < mMatrixPrimitives.size());
    assert(prim_idx < mMatrixPrimitives[idx].mPrimitives.size());
    return mMatrixPrimitives[idx].mPrimitives[prim_idx];
  }
  libcube::IndexedPrimitive &
  getMatrixPrimitiveIndexedPrimitive(u64 idx, u64 prim_idx) override {
    assert(idx < mMatrixPrimitives.size());
    assert(prim_idx < mMatrixPrimitives[idx].mPrimitives.size());
    return mMatrixPrimitives[idx].mPrimitives[prim_idx];
  }
  virtual libcube::VertexDescriptor &getVcd() override {
    return mVertexDescriptor;
  }
  virtual const libcube::VertexDescriptor &getVcd() const override {
    return mVertexDescriptor;
  }
  lib3d::AABB getBounds() const override { return bbox; }

  glm::vec2 getUv(u64 chan, u64 id) const override;
  glm::vec4 getClr(u64 id) const override;
  glm::vec3 getPos(u64 id) const override;
  glm::vec3 getNrm(u64 id) const override;
  u64 addPos(const glm::vec3 &v) override;
  u64 addNrm(const glm::vec3 &v) override;
  u64 addClr(u64 chan, const glm::vec4 &v) override;
  u64 addUv(u64 chan, const glm::vec2 &v) override;

  void addTriangle(std::array<SimpleVertex, 3> tri) override;
};

} // namespace riistudio::j3d
