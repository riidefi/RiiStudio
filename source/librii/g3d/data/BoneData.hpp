#pragma once

#include <algorithm>
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

template <typename T> bool RangeIsHomogenous(const T& range) {
  return std::adjacent_find(range.begin(), range.end(),
                            std::not_equal_to<>()) == range.end();
}

inline u32 computeFlag(const BoneData& data) {
  u32 flag = 0;
  const std::array<float, 3> scale{data.mScaling.x, data.mScaling.y,
                                   data.mScaling.z};
  if (RangeIsHomogenous(scale)) {
    flag |= 0x10;
    if (data.mScaling == glm::vec3{1.0f, 1.0f, 1.0f})
      flag |= 8;
  }
  if (data.mRotation == glm::vec3{0.0f, 0.0f, 0.0f})
    flag |= 4;
  if (data.mTranslation == glm::vec3{0.0f, 0.0f, 0.0f})
    flag |= 2;
  if (flag & (2 | 4 | 8))
    flag |= 1;
  // TODO: Flag 0x40
  if (data.ssc)
    flag |= 0x20;
  if (!data.classicScale)
    flag |= 0x80;
  if (data.visible)
    flag |= 0x100;
  // TODO: Check children?
  if (!data.mDisplayCommands.empty())
    flag |= 0x200;
  // TODO: 0x400 check parents
  return flag;
}
// Call this last
inline void setFromFlag(BoneData& data, u32 flag) {
  // TODO: Validate items
  data.ssc = (flag & 0x20) != 0;
  data.classicScale = (flag & 0x80) == 0;
  data.visible = (flag & 0x100) != 0;
}

} // namespace librii::g3d
