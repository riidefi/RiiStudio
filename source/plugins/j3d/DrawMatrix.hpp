#pragma once

#include <core/common.h>
#include <vector>

namespace riistudio::j3d {

//! Encapsulates low level envelopes and draw matrices
struct DrawMatrix {
  struct MatrixWeight {
    // TODO: Proper reference system
    u32 boneId;
    f32 weight;

    MatrixWeight() : boneId(-1), weight(0.0f) {}
    MatrixWeight(u32 b, f32 w) : boneId(b), weight(w) {}

    bool operator==(const MatrixWeight& rhs) const {
      return boneId == rhs.boneId && weight == rhs.weight;
    }
  };

  std::vector<MatrixWeight> mWeights; // 1 weight -> singlebound, no envelope

  bool operator==(const DrawMatrix& rhs) const {
    return mWeights == rhs.mWeights;
  }
};

} // namespace riistudio::j3d
