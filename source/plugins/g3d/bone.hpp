#pragma once

#include <string>
#include <vector>

#include <glm/vec3.hpp>

#include <core/3d/i3dmodel.hpp>
#include <core/common.h>
#include <librii/math/aabb.hpp>

#include <plugins/gc/Export/Bone.hpp>

namespace riistudio::g3d {

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

  u32 computeFlag() const {
    u32 flag = 0;
    if (mScaling.x == mScaling.y == mScaling.z) {
      flag |= 0x10;
      if (mScaling == glm::vec3{1.0f, 1.0f, 1.0f})
        flag |= 8;
    }
    if (mRotation == glm::vec3{0.0f, 0.0f, 0.0f})
      flag |= 4;
    if (mTranslation == glm::vec3{0.0f, 0.0f, 0.0f})
      flag |= 2;
    if (flag & (2 | 4 | 8))
      flag |= 1;
    // TODO: Flag 0x40
    if (ssc)
      flag |= 0x20;
    if (!classicScale)
      flag |= 0x80;
    if (visible)
      flag |= 0x100;
    // TODO: Check children?
    if (!mDisplayCommands.empty())
      flag |= 0x200;
    // TODO: 0x400 check parents
    return flag;
  }
  // Call this last
  void setFromFlag(u32 flag) {
    // TODO: Validate items
    ssc = (flag & 0x20) != 0;
    classicScale = (flag & 0x80) == 0;
    visible = (flag & 0x100) != 0;
  }

  glm::vec3 mScaling, mRotation, mTranslation;

  librii::math::AABB mVolume;

  s32 mParent = -1;
  std::vector<s32> mChildren;

  // TODO: userdata

  struct DisplayCommand {
    u32 mMaterial;
    u32 mPoly;
    u8 mPrio;

    bool operator==(const DisplayCommand& rhs) const = default;
  };
  std::vector<DisplayCommand> mDisplayCommands;

  std::array<f32, 3 * 4> modelMtx;
  std::array<f32, 3 * 4> inverseModelMtx;

  bool operator==(const BoneData& rhs) const = default;
};

struct Bone : public libcube::IBoneDelegate,
              public BoneData,
              public virtual kpi::IObject {
  std::string getName() const { return mName; }
  void setName(const std::string& name) override { mName = name; }
  // std::string getName() const override { return mName; }
  s64 getId() override { return id; }
  void copy(lib3d::Bone& to) const override {
    IBoneDelegate::copy(to);
    Bone* pJoint = dynamic_cast<Bone*>(&to);
    if (pJoint) {
      pJoint->mName = mName;
      pJoint->matrixId = matrixId;
      pJoint->ssc = ssc;
      pJoint->classicScale = classicScale;
      pJoint->visible = visible;
      pJoint->billboardType = billboardType;
      // billboardRefId = pJoint->billboardRefId;
    }
  }
  lib3d::Coverage supportsBoneFeature(lib3d::BoneFeatures f) override {
    switch (f) {
    case lib3d::BoneFeatures::SRT:
    case lib3d::BoneFeatures::Hierarchy:
    case lib3d::BoneFeatures::StandardBillboards:
    case lib3d::BoneFeatures::AABB:
    case lib3d::BoneFeatures::SegmentScaleCompensation:
    case lib3d::BoneFeatures::ExtendedBillboards:
      return lib3d::Coverage::ReadWrite;

    case lib3d::BoneFeatures::BoundingSphere:
    default:
      return lib3d::Coverage::Unsupported;
    }
  }

  lib3d::SRT3 getSRT() const override {
    return {mScaling, mRotation, mTranslation};
  }
  void setSRT(const lib3d::SRT3& srt) override {
    mScaling = srt.scale;
    mRotation = srt.rotation;
    mTranslation = srt.translation;
  }
  s64 getBoneParent() const override { return mParent; }
  void setBoneParent(s64 id) override { mParent = (u32)id; }
  u64 getNumChildren() const override { return mChildren.size(); }
  s64 getChild(u64 idx) const override {
    return idx < mChildren.size() ? mChildren[idx] : -1;
  }
  s64 addChild(s64 child) override {
    mChildren.push_back((u32)child);
    return mChildren.size() - 1;
  }
  s64 setChild(u64 idx, s64 id) override {
    s64 old = -1;
    if (idx < mChildren.size()) {
      old = mChildren[idx];
      mChildren[idx] = (u32)id;
    }
    return old;
  }
  librii::math::AABB getAABB() const override { return mVolume; }
  void setAABB(const librii::math::AABB& aabb) override { mVolume = aabb; }
  float getBoundingRadius() const override { return 0.0f; }
  void setBoundingRadius(float r) override {}
  bool getSSC() const override { return ssc; }
  void setSSC(bool b) override { ssc = b; }

  Billboard getBillboard() const override {
    // TODO:
    // switch (billboardType)
    { return Billboard::None; }
  }
  void setBillboard(Billboard b) override {
    //	switch (b)
    //	{
    //	case Billboard::None:
    //		bbMtxType = MatrixType::Standard;
    //		break;
    //	case Billboard::J3D_XY:
    //		bbMtxType = MatrixType::Billboard;
    //		break;
    //	case Billboard::J3D_Y:
    //		bbMtxType = MatrixType::BillboardY;
    //		return;
    //	default:
    //		break;
    //	}
  }

  u64 getNumDisplays() const override { return mDisplayCommands.size(); }
  Display getDisplay(u64 idx) const override {
    const auto& dc = mDisplayCommands[idx];

    return Display{dc.mMaterial, dc.mPoly, dc.mPrio};
  }
  void setDisplay(u64 idx, const Display& d) override {
    auto& dst = mDisplayCommands[idx];
    dst.mMaterial = d.matId;
    dst.mPoly = d.polyId;
    dst.mPrio = d.prio;
  }

  void addDisplay(const Display& d) override {
    mDisplayCommands.push_back({d.matId, d.polyId, d.prio});
  }
  bool operator==(const Bone& rhs) const {
    return static_cast<const BoneData&>(*this) == rhs;
  }
};

} // namespace riistudio::g3d
