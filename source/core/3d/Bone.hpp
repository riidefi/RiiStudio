#pragma once

#include <core/common.h>
#include <core/kpi/Node.hpp>
#include <core/kpi/Node2.hpp>
#include <glm/glm.hpp>
#include <librii/math/aabb.hpp>
#include <librii/math/srt3.hpp>
#include <string>

namespace riistudio::lib3d {

class Model;

struct Bone {
  virtual s64 getId() { return -1; }
  virtual void setName(const std::string& name) = 0;

  virtual librii::math::SRT3 getSRT() const = 0;
  virtual void setSRT(const librii::math::SRT3& srt) = 0;

  virtual s64 getBoneParent() const = 0;
  virtual void setBoneParent(s64 id) = 0;
  virtual u64 getNumChildren() const = 0;
  virtual s64 getChild(u64 idx) const = 0;
  virtual s64 addChild(s64 child) = 0;
  virtual s64 setChild(u64 idx, s64 id) = 0;
  inline s64 removeChild(u64 idx) { return setChild(idx, -1); }

  virtual librii::math::AABB getAABB() const = 0;
  virtual void setAABB(const librii::math::AABB& v) = 0;

  virtual float getBoundingRadius() const = 0;
  virtual void setBoundingRadius(float v) = 0;

  virtual bool getSSC() const = 0;
  virtual void setSSC(bool b) = 0;

  struct Display {
    u32 matId = 0;
    u32 polyId = 0;
    u8 prio = 0;

    bool operator==(const Display& rhs) const = default;
  };
  virtual u64 getNumDisplays() const = 0;
  virtual Display getDisplay(u64 idx) const = 0;
  virtual void addDisplay(const Display& d) = 0;
  virtual void setDisplay(u64 idx, const Display& d) = 0;

  virtual glm::mat4
  calcSrtMtx(kpi::ConstCollectionRange<lib3d::Bone> bones) const {
    glm::mat4 mdl(1.0f);
    const auto parent = getBoneParent();
    if (parent >= 0 /* && parent != getId() */)
      mdl = bones[parent].calcSrtMtx(bones);

    return mdl * librii::math::calcXform(getSRT());
  }
  glm::mat4 calcSrtMtx(const lib3d::Model* mdl) const;
};

} // namespace riistudio::lib3d
