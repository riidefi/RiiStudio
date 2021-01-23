#pragma once

#include "aabb.hpp"
#include <array>
#include <core/common.h>
#include <core/kpi/Node2.hpp>
#include <glm/glm.hpp>
#include <librii/glhelper/UBOBuilder.hpp>
#include <librii/glhelper/VBOBuilder.hpp>
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

  // For now... (slow api)
  virtual void propagate(const Model& mdl, u32 mp_id,
                         librii::glhelper::SpliceVBOBuilder& out) const = 0;

  // Call after any change
  virtual void update() {}

  virtual AABB getBounds() const = 0;

  bool is_xlu_import = false;
};

} // namespace riistudio::lib3d
