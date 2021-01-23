#pragma once

#include <core/3d/i3dmodel.hpp>
#include <core/common.h>
#include <librii/gx.h>

namespace libcube {

class Model;

struct IndexedPolygon : public riistudio::lib3d::Polygon {
  virtual void setId(u32 id) = 0;

  virtual librii::gx::MeshData& getMeshData() = 0;
  virtual const librii::gx::MeshData& getMeshData() const = 0;

  bool hasAttrib(SimpleAttrib attrib) const override;
  void setAttrib(SimpleAttrib attrib, bool v) override;
  void propagate(const riistudio::lib3d::Model& mdl, u32 mp_id,
                 librii::glhelper::SpliceVBOBuilder& out) const override;
  virtual glm::vec3 getPos(const Model& mdl, u64 id) const = 0;
  virtual glm::vec3 getNrm(const Model& mdl, u64 id) const = 0;
  virtual glm::vec4 getClr(const Model& mdl, u64 chan, u64 id) const = 0;
  virtual glm::vec2 getUv(const Model& mdl, u64 chan, u64 id) const = 0;

  virtual u64 addPos(libcube::Model& mdl, const glm::vec3& v) = 0;
  virtual u64 addNrm(libcube::Model& mdl, const glm::vec3& v) = 0;
  virtual u64 addClr(libcube::Model& mdl, u64 chan, const glm::vec4& v) = 0;
  virtual u64 addUv(libcube::Model& mdl, u64 chan, const glm::vec2& v) = 0;

  void update() override {
    // Split up added primitives if necessary
  }

  virtual librii::gx::VertexDescriptor& getVcd() {
    return getMeshData().mVertexDescriptor;
  }
  virtual const librii::gx::VertexDescriptor& getVcd() const {
    return getMeshData().mVertexDescriptor;
  }

  virtual std::vector<glm::mat4> getPosMtx(const libcube::Model& mdl,
                                           u64 mpId) const {
    return {};
  }

  virtual void init(bool skinned, riistudio::lib3d::AABB* boundingBox) = 0;
  virtual void initBufsFromVcd(riistudio::lib3d::Model&) {}
};

} // namespace libcube
