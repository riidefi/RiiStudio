#pragma once

#include <algorithm>
#include <array>
#include <core/common.h>
#include <glm/vec3.hpp>
#include <librii/math/aabb.hpp>
#include <span>
#include <string>
#include <vector>

#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>

namespace librii::g3d {

class NameTable;

struct BoneData;

struct BinaryBoneData {
  std::string name;
  u32 flag = 0;
  u32 id = 0;
  u32 matrixId = 0;
  u32 billboardType = 0;
  u32 ancestorBillboardBone = 0;
  glm::vec3 scale{1.0f, 1.0f, 1.0f};
  glm::vec3 rotate{0.0f, 0.0f, 0.0f};
  glm::vec3 translate{0.0f, 0.0f, 0.0f};
  librii::math::AABB aabb;
  int parent_id = -1;
  int child_first_id = -1;
  // Non-cyclic linked-list
  int sibling_right_id = -1;
  int sibling_left_id = -1;
  std::array<f32, 3 * 4> modelMtx;
  std::array<f32, 3 * 4> inverseModelMtx;

  void read(oishii::BinaryReader& reader);
  void write(NameTable& names, oishii::Writer& writer, u32 mdl_start) const;
};

struct BoneData {
  std::string mName = "Untitled Bone";

  bool ssc = false;
  bool classicScale = true;
  bool visible = true;

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

  std::string getName() const { return mName; }

  bool operator==(const BoneData& rhs) const = default;
};

} // namespace librii::g3d
