#pragma once

#include "aabb.hpp"
#include <array>
#include <core/3d/renderer/UBOBuilder.hpp>
#include <core/3d/renderer/VBOBuilder.hpp>
#include <core/common.h>
#include <core/kpi/Node2.hpp>
#include <glm/glm.hpp>
#include <string>

namespace riistudio::lib3d {

struct Polygon : public virtual kpi::IObject {
  virtual ~Polygon() = default;

  virtual bool isVisible() const { return true; }
  virtual std::string getName() const { return "TODO"; }
  virtual void setName(const std::string& name) = 0;

  enum class SimpleAttrib {
    EnvelopeIndex, // u8
    Position,      // vec3
    Normal,        // vec3
    Color0,        // rgba8
    Color1,
    TexCoord0, // vec2
    TexCoord1,
    TexCoord2,
    TexCoord3,
    TexCoord4,
    TexCoord5,
    TexCoord6,
    TexCoord7,
    Max
  };
  virtual bool hasAttrib(SimpleAttrib attrib) const = 0;
  virtual void setAttrib(SimpleAttrib attrib, bool v) = 0;

  struct SimpleVertex {
    // Not all are real. Use hasAttrib()
    u8 evpIdx; // If read from a GC model, not local to mprim
    glm::vec3 position;
    glm::vec3 normal;
    std::array<glm::vec4, 2> colors;
    std::array<glm::vec2, 8> uvs;
  };

  // For now... (slow api)
  virtual void propogate(VBOBuilder& out) const = 0;

  virtual void addTriangle(std::array<SimpleVertex, 3> tri) = 0;
  //	virtual SimpleVertex getPrimitiveVertex(u64 prim_idx, u64 vtx_idx) = 0;
  //	virtual void setPrimitiveVertex(u64 prim_idx, u64 vtx_idx, const
  // SimpleVertex& vtx) = 0;

  // Call after any change
  virtual void update() {}

  virtual AABB getBounds() const = 0;

  bool is_xlu_import = false;
};

} // namespace riistudio::lib3d
