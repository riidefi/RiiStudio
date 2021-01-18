#pragma once

#include <core/common.h>
#include <librii/gx.h>
#include <plugins/gc/Export/IndexedPolygon.hpp>
#include <vector>

#include <core/kpi/Node.hpp>

namespace riistudio::g3d {

class Model;

using MatrixPrimitive = librii::gx::MatrixPrimitive;
struct PolygonData : public librii::gx::MeshData {
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

  bool operator==(const PolygonData& rhs) const = default;
};
struct Polygon : public PolygonData,
                 public libcube::IndexedPolygon,
                 public virtual kpi::IObject {
  void setId(u32 id) override { mId = id; }
  virtual const g3d::Model* getParent() const;
  g3d::Model* getMutParent();
  std::string getName() const { return mName; }
  void setName(const std::string& name) override { mName = name; }

  MeshData& getMeshData() override { return *this; }
  const MeshData& getMeshData() const { return *this; }
  lib3d::AABB getBounds() const override { return bounds; }
  std::vector<glm::mat4> getPosMtx(u64 mpId) const override;

  glm::vec2 getUv(u64 chan, u64 id) const override;
  glm::vec4 getClr(u64 chan, u64 id) const override;
  glm::vec3 getPos(u64 id) const override;
  glm::vec3 getNrm(u64 id) const override;
  u64 addPos(const glm::vec3& v) override;
  u64 addNrm(const glm::vec3& v) override;
  u64 addClr(u64 chan, const glm::vec4& v) override;
  u64 addUv(u64 chan, const glm::vec2& v) override;

  void init(bool skinned, riistudio::lib3d::AABB* boundingBox) override {
    // TODO: Handle skinning...
    (void)skinned;
    if (boundingBox != nullptr)
      bounds = *boundingBox;
  }
  void initBufsFromVcd() override;
  bool operator==(const Polygon& rhs) const {
    return PolygonData::operator==(rhs);
  }

  lib3d::AABB bounds;
};

} // namespace riistudio::g3d
