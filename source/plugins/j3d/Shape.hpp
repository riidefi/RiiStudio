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
  virtual const j3d::Model* getParent() const { return nullptr; }

  std::string getName() const override { return "Shape " + std::to_string((u64)this); }
  void setName(const std::string& name) override {}

  MeshData& getMeshData() override { return *this; }
  const MeshData& getMeshData() const { return *this; }
  librii::math::AABB getBounds() const override { return bbox; }

  std::span<const glm::vec2> getUv(const libcube::Model& mdl,
                                   u64 chan) const override;
  std::span<const librii::gx::Color> getClr(const libcube::Model& mdl,
                                            u64 chan) const override;
  std::span<const glm::vec3> getPos(const libcube::Model& mdl) const override;
  std::span<const glm::vec3> getNrm(const libcube::Model& mdl) const override;
  u64 addPos(libcube::Model& mdl, const glm::vec3& v) override;
  u64 addNrm(libcube::Model& mdl, const glm::vec3& v) override;
  u64 addClr(libcube::Model& mdl, u64 chan, const glm::vec4& v) override;
  u64 addUv(libcube::Model& mdl, u64 chan, const glm::vec2& v) override;

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
