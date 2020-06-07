#pragma once

#include "aabb.hpp"
#include <core/3d/PropertySupport.hpp>
#include <core/common.h>
#include <core/kpi/Node.hpp>
#include <glm/glm.hpp>
#include <string>

namespace riistudio::lib3d {

enum class BoneFeatures {
  // -- Standard features: XForm, Hierarchy. Here for read-only access
  SRT,
  Hierarchy,
  // -- Optional features
  StandardBillboards, // J3D
  ExtendedBillboards, // G3D
  AABB,
  BoundingSphere,
  SegmentScaleCompensation, // Maya
                            // Not exposed currently:
                            //	ModelMatrix,
                            //	InvModelMatrix
  Max
};
struct SRT3 {
  glm::vec3 scale;
  glm::vec3 rotation;
  glm::vec3 translation;

  bool operator==(const SRT3& rhs) const {
    return scale == rhs.scale && rotation == rhs.rotation &&
           translation == rhs.translation;
  }
  bool operator!=(const SRT3& rhs) const { return !operator==(rhs); }
};

struct Bone {
  // // PX_TYPE_INFO_EX("3D Bone", "3d_bone", "3D::Bone", ICON_FA_BONE,
  // ICON_FA_BONE);
  virtual s64 getId() { return -1; }
  virtual void setName(const std::string& name) = 0;
  inline virtual void copy(lib3d::Bone& to) const {
    to.setSRT(getSRT());
    to.setBoneParent(getBoneParent());
    to.setSSC(getSSC());
    to.setAABB(getAABB());
    to.setBoundingRadius(getBoundingRadius());
    // TODO: The rest
    for (int i = 0; i < getNumDisplays(); ++i) {
      const auto d = getDisplay(i);
      to.addDisplay(d);
    }
  }

  virtual Coverage supportsBoneFeature(BoneFeatures f) {
    return Coverage::Unsupported;
  }

  virtual SRT3 getSRT() const = 0;
  virtual void setSRT(const SRT3& srt) = 0;

  virtual s64 getBoneParent() const = 0;
  virtual void setBoneParent(s64 id) = 0;
  virtual u64 getNumChildren() const = 0;
  virtual s64 getChild(u64 idx) const = 0;
  virtual s64 addChild(s64 child) = 0;
  virtual s64 setChild(u64 idx, s64 id) = 0;
  inline s64 removeChild(u64 idx) { return setChild(idx, -1); }

  virtual AABB getAABB() const = 0;
  virtual void setAABB(const AABB& v) = 0;

  virtual float getBoundingRadius() const = 0;
  virtual void setBoundingRadius(float v) = 0;

  virtual bool getSSC() const = 0;
  virtual void setSSC(bool b) = 0;

  struct Display {
    u32 matId = 0;
    u32 polyId = 0;
    u8 prio = 0;

    bool operator==(const Display& rhs) const {
      return matId == rhs.matId && polyId == rhs.polyId && prio == rhs.prio;
    }
  };
  virtual u64 getNumDisplays() const = 0;
  virtual Display getDisplay(u64 idx) const = 0;
  virtual void addDisplay(const Display& d) = 0;
  virtual void setDisplay(u64 idx, const Display& d) = 0;

  inline glm::mat4 calcSrtMtx(const lib3d::SRT3& srt) const {
    glm::mat4 dst(1.0f);

    //	dst = glm::translate(dst, srt.translation);
    //	dst = dst * glm::eulerAngleZYX(glm::radians(srt.rotation.x),
    // glm::radians(srt.rotation.y), glm::radians(srt.rotation.z)); 	return
    // glm::scale(dst, srt.scale);

    float sinX = sin(glm::radians(srt.rotation.x)),
          cosX = cos(glm::radians(srt.rotation.x));
    float sinY = sin(glm::radians(srt.rotation.y)),
          cosY = cos(glm::radians(srt.rotation.y));
    float sinZ = sin(glm::radians(srt.rotation.z)),
          cosZ = cos(glm::radians(srt.rotation.z));

    dst[0][0] = srt.scale.x * (cosY * cosZ);
    dst[0][1] = srt.scale.x * (sinZ * cosY);
    dst[0][2] = srt.scale.x * (-sinY);
    dst[0][3] = 0.0;
    dst[1][0] = srt.scale.y * (sinX * cosZ * sinY - cosX * sinZ);
    dst[1][1] = srt.scale.y * (sinX * sinZ * sinY + cosX * cosZ);
    dst[1][2] = srt.scale.y * (sinX * cosY);
    dst[1][3] = 0.0;
    dst[2][0] = srt.scale.z * (cosX * cosZ * sinY + sinX * sinZ);
    dst[2][1] = srt.scale.z * (cosX * sinZ * sinY - sinX * cosZ);
    dst[2][2] = srt.scale.z * (cosY * cosX);
    dst[2][3] = 0.0;
    dst[3][0] = srt.translation.x;
    dst[3][1] = srt.translation.y;
    dst[3][2] = srt.translation.z;
    dst[3][3] = 1.0;

    return dst;
  }
  virtual kpi::IDocumentNode* getParent() { return nullptr; }
  virtual glm::mat4 calcSrtMtx(kpi::FolderData* bones) const {
    glm::mat4 mdl(1.0f);
    const auto parent = getBoneParent();
    if (parent >= 0 /* && parent != getId() */)
      mdl = bones->at<Bone>(parent).calcSrtMtx(bones);

    return mdl * calcSrtMtx(getSRT());
  }
  inline glm::mat4 calcSrtMtx() {
    assert(getParent() != nullptr);
    if (!getParent())
      return {};

    return calcSrtMtx(getParent()->getFolder<lib3d::Bone>());
  }
};

} // namespace riistudio::lib3d
