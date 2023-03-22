#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include <glm/vec3.hpp>

#include <core/3d/i3dmodel.hpp>
#include <core/common.h>
#include <librii/math/aabb.hpp>

#include <librii/g3d/data/BoneData.hpp>
#include <plugins/gc/Export/Bone.hpp>

namespace riistudio::g3d {


struct Bone : public libcube::IBoneDelegate,
              public librii::g3d::BoneData,
              public virtual kpi::IObject {
  std::string getName() const { return mName; }
  void setName(const std::string& name) override { mName = name; }
  // std::string getName() const override { return mName; }
  s64 getId() override { return id; }

  librii::math::SRT3 getSRT() const override {
    return {mScaling, mRotation, mTranslation};
  }
  void setSRT(const librii::math::SRT3& srt) override {
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
    billboardType = (int)b;
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
