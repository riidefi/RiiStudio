#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include <glm/vec3.hpp>

#include <core/common.h>
#include <librii/math/aabb.hpp>
#include <plugins/3d/i3dmodel.hpp>

#include <librii/g3d/data/BoneData.hpp>
#include <plugins/gc/Export/Bone.hpp>

namespace riistudio::g3d {

struct Bone : public libcube::IBoneDelegate,
              private librii::g3d::BoneData,
              public virtual kpi::IObject {
  std::string getName() const override { return mName; }
  void setName(const std::string& name) override { mName = name; }
  // std::string getName() const override { return mName; }

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
    { return static_cast<Billboard>(billboardType); }
  }
  void setBillboard(Billboard b) override { billboardType = (int)b; }

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
  void clearDrawCalls() { mDisplayCommands.clear(); }
  bool operator==(const Bone& rhs) const {
    return static_cast<const BoneData&>(*this) == rhs;
  }

  void decompile(const librii::g3d::BoneData& b) {
    static_cast<librii::g3d::BoneData&>(*this) = b;
  }
  const librii::g3d::BoneData& compile() const { return *this; }
};

} // namespace riistudio::g3d
