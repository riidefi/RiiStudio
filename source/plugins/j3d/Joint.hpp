#pragma once

#include <core/common.h>
#include <glm/glm.hpp>
#include <librii/j3d/data/JointData.hpp>
#include <plugins/3d/i3dmodel.hpp>
#include <plugins/gc/Export/Bone.hpp>
#include <string>
#include <vector>

namespace riistudio::j3d {

struct Joint : public libcube::IBoneDelegate,
               private /* mutable */ librii::j3d::JointData,
               public virtual kpi::IObject {
  // PX_TYPE_INFO_EX("J3D Joint", "j3d_joint", "J::Joint", ICON_FA_BONE,
  // ICON_FA_BONE);

  std::string getName() const override { return name; }
  void setName(const std::string& n) override { name = n; }

  librii::math::SRT3 getSRT() const override {
    return {scale, rotate, translate};
  }
  void setSRT(const librii::math::SRT3& srt) override {
    scale = srt.scale;
    rotate = srt.rotation;
    translate = srt.translation;
  }
  s64 getBoneParent() const override { return parentId; }
  void setBoneParent(s64 id) override { parentId = (u32)id; }
  u64 getNumChildren() const override { return children.size(); }
  s64 getChild(u64 idx) const override {
    return idx < children.size() ? children[idx] : -1;
  }
  s64 addChild(s64 child) override {
    children.push_back((u32)child);
    return children.size() - 1;
  }
  s64 setChild(u64 idx, s64 id) override {
    s64 old = -1;
    if (idx < children.size()) {
      old = children[idx];
      children[idx] = (u32)id;
    }
    return old;
  }
  librii::math::AABB getAABB() const override { return boundingBox; }
  void setAABB(const librii::math::AABB& aabb) override { boundingBox = aabb; }
  float getBoundingRadius() const override { return boundingSphereRadius; }
  void setBoundingRadius(float r) override { boundingSphereRadius = r; }
  bool getSSC() const override { return mayaSSC; }
  void setSSC(bool b) override { mayaSSC = b; }

  Billboard getBillboard() const override {
    switch (bbMtxType) {
    case librii::j3d::MatrixType::Standard:
      return Billboard::None;
    case librii::j3d::MatrixType::Billboard:
      return Billboard::J3D_XY;
    case librii::j3d::MatrixType::BillboardY:
      return Billboard::J3D_Y;
    default:
      return Billboard::None;
    }
  }
  void setBillboard(Billboard b) override {
    switch (b) {
    case Billboard::None:
      bbMtxType = librii::j3d::MatrixType::Standard;
      break;
    case Billboard::J3D_XY:
      bbMtxType = librii::j3d::MatrixType::Billboard;
      break;
    case Billboard::J3D_Y:
      bbMtxType = librii::j3d::MatrixType::BillboardY;
      return;
    default:
      break;
    }
  }
  u64 getNumDisplays() const override { return displays.size(); }
  lib3d::Bone::Display getDisplay(u64 idx) const override {
    return {(u32)displays[idx].material, (u32)displays[idx].shape};
  }
  void setDisplay(u64 idx, const lib3d::Bone::Display& d) override {
    auto& dst = displays[idx];
    dst.material = d.matId;
    dst.shape = d.polyId;
  }

  void addDisplay(const lib3d::Bone::Display& d) override {
    displays.emplace_back(d.matId, d.polyId);
  }

  void decompile(const librii::j3d::JointData& b) {
    static_cast<librii::j3d::JointData&>(*this) = b;
    j_flag = flag;
    j_bbMtxType = bbMtxType;
    j_mayaSSC = mayaSSC;
    j_boundingSphereRadius = boundingSphereRadius;
    j_boundingBox = boundingBox;
  }
  const librii::j3d::JointData& compile() {
    flag = j_flag;
    bbMtxType = j_bbMtxType;
    mayaSSC = j_mayaSSC;
    boundingSphereRadius = j_boundingSphereRadius;
    boundingBox = j_boundingBox;
    return *this;
  }
  const librii::j3d::JointData& compile() const {
    // We treat the JointData parent as a mutable member
    return const_cast<Joint*>(this)->compile();
  }

  u16 j_flag = 1; // Unused four bits; default value in galaxy is 1
  librii::j3d::MatrixType j_bbMtxType = librii::j3d::MatrixType::Standard;
  bool j_mayaSSC = false;
  f32 j_boundingSphereRadius = 100000.0f;
  librii::math::AABB j_boundingBox{{-100000.0f, -100000.0f, -100000.0f},
                                   {100000.0f, 100000.0f, 100000.0f}};
  bool operator==(const Joint& rhs) const {
    // We treat the JointData parent as a mutable member
    const_cast<Joint*>(this)->compile();
    return static_cast<const JointData&>(*this) == rhs &&
           j_flag == rhs.j_flag && j_bbMtxType == rhs.j_bbMtxType &&
           j_mayaSSC == rhs.j_mayaSSC &&
           j_boundingSphereRadius == rhs.j_boundingSphereRadius &&
           j_boundingBox == rhs.j_boundingBox;
  }
};

} // namespace riistudio::j3d
