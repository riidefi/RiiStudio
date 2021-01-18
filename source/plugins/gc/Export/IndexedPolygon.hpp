#pragma once

#include <core/3d/i3dmodel.hpp>
#include <core/common.h>
#include <librii/gx.h>

namespace libcube {

struct IndexedPolygon : public riistudio::lib3d::Polygon {
  virtual void setId(u32 id) = 0;

  virtual librii::gx::MeshData& getMeshData() = 0;
  virtual const librii::gx::MeshData& getMeshData() const = 0;

  bool hasAttrib(SimpleAttrib attrib) const override;
  void setAttrib(SimpleAttrib attrib, bool v) override;
  void propogate(VBOBuilder& out) const override;
  virtual glm::vec3 getPos(u64 id) const = 0;
  virtual glm::vec3 getNrm(u64 id) const = 0;
  virtual glm::vec4 getClr(u64 chan, u64 id) const = 0;
  virtual glm::vec2 getUv(u64 chan, u64 id) const = 0;

  virtual u64 addPos(const glm::vec3& v) = 0;
  virtual u64 addNrm(const glm::vec3& v) = 0;
  virtual u64 addClr(u64 chan, const glm::vec4& v) = 0;
  virtual u64 addUv(u64 chan, const glm::vec2& v) = 0;

  void update() override {
    // Split up added primitives if necessary
  }

  virtual librii::gx::VertexDescriptor& getVcd() {
    return getMeshData().mVertexDescriptor;
  }
  virtual const librii::gx::VertexDescriptor& getVcd() const {
    return getMeshData().mVertexDescriptor;
  }

  virtual std::vector<glm::mat4> getPosMtx(u64 mpId) const { return {}; }

  virtual void init(bool skinned, riistudio::lib3d::AABB* boundingBox) = 0;
  virtual void initBufsFromVcd() {}
};

} // namespace libcube
