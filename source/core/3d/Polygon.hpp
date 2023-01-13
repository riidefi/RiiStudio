#pragma once

#include <array>
#include <core/common.h>
#include <core/kpi/Node2.hpp>
#include <glm/glm.hpp>
#include <librii/glhelper/VBOBuilder.hpp>
#include <librii/math/aabb.hpp>
#include <string>

namespace riistudio::lib3d {

struct Bone;
class Model;

struct IndexRange {
  u32 start = 0;
  u32 size = 0;
};

struct Polygon : public virtual kpi::IObject {
  virtual ~Polygon() = default;

  virtual bool isVisible() const { return true; }
  virtual std::string getName() const override { return "TODO"; }
  virtual void setName(const std::string& name) = 0;

  // For now... (slow api)
  virtual std::expected<riistudio::lib3d::IndexRange, std::string>
  propagate(const Model& mdl, u32 mp_id,
            librii::glhelper::VBOBuilder& out) const = 0;

  // Call after any change
  virtual void update() {}

  virtual librii::math::AABB getBounds() const = 0;

  bool is_xlu_import = false;
  bool is_local_space = false;
  bool is_mask = false;

  virtual s32 getGenerationId() const { return mGenerationId; }
  virtual void nextGenerationId() { ++mGenerationId; }

  s32 mGenerationId = 0;
};

// Calculates a *transformed* version of the polygon's bounding box, accounting
// for the deformation of the bone and its parents.
librii::math::AABB CalcPolyBound(const lib3d::Polygon& poly,
                                 const lib3d::Bone& bone,
                                 const lib3d::Model& mdl);

} // namespace riistudio::lib3d
