#pragma once

#include <plugins/3d/i3dmodel.hpp>
#include <core/common.h>

// #include <LibCore/api/Node.hpp>

namespace libcube {
//! Encapsulates low level envelopes and draw matrices
struct DrawMatrix {
  struct MatrixWeight {
    // TODO: Proper reference system
    u32 boneId;
    f32 weight;

    MatrixWeight() : boneId(-1), weight(0.0f) {}
    MatrixWeight(u32 b, f32 w) : boneId(b), weight(w) {}

    bool operator==(const MatrixWeight& rhs) const = default;
  };

  std::vector<MatrixWeight> mWeights; // 1 weight -> singlebound, no envelope

  bool operator==(const DrawMatrix& rhs) const = default;
};
struct ModelData {
  std::vector<DrawMatrix> mDrawMatrices;

  bool operator==(const ModelData& rhs) const = default;
};

struct IBoneDelegate : public riistudio::lib3d::Bone {
  // PX_TYPE_INFO("GC Bone", "gc_bone", "GC::IBoneDelegate");

  enum class Billboard {
    None,
    Z_Parallel,
    Z_Face,
    Z_ParallelRotate,
    Z_FaceRotate,
    Y_Parallel,
    Y_Face,

    // TODO -- merge
    J3D_XY,
    J3D_Y
  };
  virtual Billboard getBillboard() const = 0;
  virtual void setBillboard(Billboard b) = 0;

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
};

} // namespace libcube
