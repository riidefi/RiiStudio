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

#include <glm/mat4x3.hpp>

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

  // 3 columns 4 rows (Mtx34 in-game)
  glm::mat4x3 modelMtx;
  glm::mat4x3 inverseModelMtx;

  // Not a real field: computed later on
  bool forceDisplayMatrix = false;
  bool omitFromNodeMix = false;

  Result<void> read(oishii::BinaryReader& reader);
  void write(NameTable& names, oishii::Writer& writer, u32 mdl_start) const;

  bool isDisplayMatrix() const { return flag & 0x200; }
  bool hasChildren() const { return child_first_id != -1; }
};

struct BoneData {
  std::string mName = "Untitled Bone";

  bool ssc = false;
  // Instead refer to scalingRule != SOFTIMAGE
  // bool classicScale = true;
  bool visible = true;

  // Recomputed
  // u32 matrixId = 0;
  u32 billboardType = 0;

  glm::vec3 mScaling{1.0f, 1.0f, 1.0f};
  glm::vec3 mRotation{0.0f, 0.0f, 0.0f};
  glm::vec3 mTranslation{0.0f, 0.0f, 0.0f};

  librii::math::AABB mVolume;

  s32 mParent = -1;
  std::vector<s32> mChildren;

  bool displayMatrix = true;
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

  // Not a real field: computed later on
  bool forceDisplayMatrix = false;
  bool omitFromNodeMix = false;

  std::string getName() const { return mName; }
  bool isDisplayMatrix() const { return displayMatrix; }
  bool hasChildren() const { return !mChildren.empty(); }

  bool operator==(const BoneData& rhs) const = default;
};

} // namespace librii::g3d
