#pragma once

#include <core/3d/i3dmodel.hpp>
#include <core/common.h>

// #include <LibCore/api/Node.hpp>

namespace libcube {

struct IBoneDelegate : public riistudio::lib3d::Bone {
  // PX_TYPE_INFO("GC Bone", "gc_bone", "GC::IBoneDelegate");

  enum class Billboard {
    None,
    /* G3D Standard */
    Z_Parallel,
    Z_Face, // persp
    Z_ParallelRotate,
    Z_FaceRotate,
    /* G3D Y*/
    Y_Parallel,
    Y_Face, // persp

    // TODO -- merge
    J3D_XY,
    J3D_Y
  };
  virtual Billboard getBillboard() const = 0;
  virtual void setBillboard(Billboard b) = 0;
  // For extendeds
  virtual s64 getBillboardAncestor() const { return -1; }
  virtual void setBillboardAncestor(s64 ancestor_id) {}

  glm::vec3 getScale() const { return getSRT().scale; }
  glm::vec3 getRotation() const { return getSRT().rotation; }
  glm::vec3 getTranslation() const { return getSRT().translation; }
  void setScale(glm::vec3 s) {
    auto srt = getSRT();
    srt.scale = s;
    setSRT(srt);
  }
  void setRotation(glm::vec3 r) {
    auto srt = getSRT();
    srt.rotation = r;
    setSRT(srt);
  }
  void setTranslation(glm::vec3 t) {
    auto srt = getSRT();
    srt.translation = t;
    setSRT(srt);
  }

  void copy(riistudio::lib3d::Bone& to) const override {
    riistudio::lib3d::Bone::copy(to);
    IBoneDelegate* bone = reinterpret_cast<IBoneDelegate*>(&to);
    if (bone) {
      if (bone->supportsBoneFeature(
              riistudio::lib3d::BoneFeatures::StandardBillboards) ==
          riistudio::lib3d::Coverage::ReadWrite)
        bone->setBillboard(getBillboard());
      if (bone->supportsBoneFeature(
              riistudio::lib3d::BoneFeatures::ExtendedBillboards) ==
          riistudio::lib3d::Coverage::ReadWrite)
        bone->setBillboardAncestor(getBillboardAncestor());
    }
  }
  inline bool canWrite(riistudio::lib3d::BoneFeatures f) {
    return supportsBoneFeature(f) == riistudio::lib3d::Coverage::ReadWrite;
  }
};

} // namespace libcube
