#pragma once

#include <array>
#include <core/common.h>
#include <glm/vec3.hpp>
#include <librii/math/aabb.hpp>
#include <string>
#include <vector>

namespace librii::g3d {

struct BoneData {
  std::string mName = "Untitled Bone";

  // Flags:
  // SRT checks -- recompute
  bool ssc = false;
  // SSC parent - recompute
  bool classicScale = true;
  bool visible = true;
  // display mtx - recompute
  // bb ref recomputed

  u32 matrixId = 0;
  u32 billboardType = 0;

  glm::vec3 mScaling{1.0f, 1.0f, 1.0f};
  glm::vec3 mRotation{0.0f, 0.0f, 0.0f};
  glm::vec3 mTranslation{0.0f, 0.0f, 0.0f};

  librii::math::AABB mVolume;

  s32 mParent = -1;
  std::vector<s32> mChildren;

  // TODO: userdata

  struct DisplayCommand {
    u32 mMaterial;
    u32 mPoly;
    u8 mPrio;

    bool operator==(const DisplayCommand& rhs) const = default;
    bool operator<(const DisplayCommand& rhs) const {
      if (mPrio != rhs.mPrio)
        return mPrio < rhs.mPrio;
      if (mPoly != rhs.mPoly)
        return mPoly < rhs.mPoly;
      return mMaterial < rhs.mMaterial;
    }
  };
  std::vector<DisplayCommand> mDisplayCommands;

  std::array<f32, 3 * 4> modelMtx;
  std::array<f32, 3 * 4> inverseModelMtx;

  bool operator==(const BoneData& rhs) const = default;
};

} // namespace librii::g3d
