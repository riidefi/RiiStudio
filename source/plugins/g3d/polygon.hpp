#pragma once

#include <core/common.h>
#include <librii/gx.h>
#include <plugins/gc/Export/IndexedPolygon.hpp>
#include <vector>

#include <LibBadUIFramework/Node2.hpp>

#include <librii/g3d/data/PolygonData.hpp>

namespace riistudio::g3d {

class Model;

using MatrixPrimitive = librii::gx::MatrixPrimitive;
struct Polygon : public librii::g3d::PolygonData,
                 public libcube::IndexedPolygon,
                 public virtual kpi::IObject {
  std::string getName() const { return mName; }
  void setName(const std::string& name) override { mName = name; }

  MeshData& getMeshData() override { return *this; }
  const MeshData& getMeshData() const { return *this; }
  librii::math::AABB getBounds() const override { return bounds; }

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

  void init(bool skinned, librii::math::AABB* boundingBox) override {
    // TODO: Handle skinning...
    (void)skinned;
    if (boundingBox != nullptr)
      bounds = *boundingBox;
  }
  void initBufsFromVcd(lib3d::Model& mdl) override;
  bool operator==(const Polygon& rhs) const {
    return PolygonData::operator==(rhs);
  }

  void setCurMtx(s32 mtx) override { mCurrentMatrix = static_cast<s16>(mtx); }

  librii::math::AABB bounds;
};

} // namespace riistudio::g3d
