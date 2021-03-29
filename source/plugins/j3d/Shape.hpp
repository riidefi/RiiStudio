#pragma once

#include <core/common.h>
#include <librii/gx.h>
#include <librii/j3d/data/ShapeData.hpp>
#include <plugins/gc/Export/IndexedPolygon.hpp>
#include <vector>

namespace riistudio::j3d {

class Model;

using MatrixPrimitive = librii::gx::MatrixPrimitive;

struct Shape : public librii::j3d::ShapeData,
               public libcube::IndexedPolygon,
               public virtual kpi::IObject {
  void setId(u32 _id) override { id = _id; }
  virtual const j3d::Model* getParent() const { return nullptr; }

  std::string getName() const { return "Shape " + std::to_string(id); }
  void setName(const std::string& name) override {}

  MeshData& getMeshData() override { return *this; }
  const MeshData& getMeshData() const { return *this; }
  librii::math::AABB getBounds() const override { return bbox; }

  glm::vec2 getUv(const libcube::Model& mdl, u64 chan, u64 id) const override;
  glm::vec4 getClr(const libcube::Model& mdl, u64 chan, u64 id) const override;
  glm::vec3 getPos(const libcube::Model& mdl, u64 id) const override;
  glm::vec3 getNrm(const libcube::Model& mdl, u64 id) const override;
  u64 addPos(libcube::Model& mdl, const glm::vec3& v) override;
  u64 addNrm(libcube::Model& mdl, const glm::vec3& v) override;
  u64 addClr(libcube::Model& mdl, u64 chan, const glm::vec4& v) override;
  u64 addUv(libcube::Model& mdl, u64 chan, const glm::vec2& v) override;

  std::vector<glm::mat4> getPosMtx(const libcube::Model& mdl,
                                   u64 mpid) const override;

  bool isVisible() const override { return visible; }
  void init(bool skinned, librii::math::AABB* boundingBox) override {
    if (skinned)
      mode = librii::j3d::ShapeData::Mode::Skinned;
    if (boundingBox != nullptr)
      bbox = *boundingBox;
  }

  bool visible = true; // Editor-only
};

} // namespace riistudio::j3d
