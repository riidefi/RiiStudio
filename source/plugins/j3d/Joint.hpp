#pragma once

#include <core/common.h>
#include <glm/glm.hpp>
#include <librii/j3d/data/JointData.hpp>
#include <plugins/3d/i3dmodel.hpp>
#include <plugins/gc/Export/Bone.hpp>
#include <string>
#include <vector>

namespace riistudio::j3d {

class Joint : public libcube::IBoneDelegate, public virtual kpi::IObject {
public:
  std::string getName() const override { return m_jointData.name; }
  void setName(const std::string& n) override { m_jointData.name = n; }

  librii::math::SRT3 getSRT() const override {
    return {m_jointData.scale, m_jointData.rotate, m_jointData.translate};
  }
  void setSRT(const librii::math::SRT3& srt) override {
    m_jointData.scale = srt.scale;
    m_jointData.rotate = srt.rotation;
    m_jointData.translate = srt.translation;
  }
  s64 getBoneParent() const override { return m_jointData.parentId; }
  void setBoneParent(s64 id) override { m_jointData.parentId = (u32)id; }
  u64 getNumChildren() const override { return m_jointData.children.size(); }
  s64 getChild(u64 idx) const override {
    return idx < m_jointData.children.size() ? m_jointData.children[idx] : -1;
  }
  s64 addChild(s64 child) override {
    m_jointData.children.push_back((u32)child);
    return m_jointData.children.size() - 1;
  }
  s64 setChild(u64 idx, s64 id) override {
    s64 old = -1;
    if (idx < m_jointData.children.size()) {
      old = m_jointData.children[idx];
      m_jointData.children[idx] = (u32)id;
    }
    return old;
  }
  librii::math::AABB getAABB() const override { return j_boundingBox; }
  void setAABB(const librii::math::AABB& aabb) override {
    j_boundingBox = aabb;
  }
  float getBoundingRadius() const override { return j_boundingSphereRadius; }
  void setBoundingRadius(float r) override { j_boundingSphereRadius = r; }
  bool getSSC() const override { return j_mayaSSC; }
  void setSSC(bool b) override { j_mayaSSC = b; }

  Billboard getBillboard() const override {
    switch (m_jointData.bbMtxType) {
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
      m_jointData.bbMtxType = librii::j3d::MatrixType::Standard;
      break;
    case Billboard::J3D_XY:
      m_jointData.bbMtxType = librii::j3d::MatrixType::Billboard;
      break;
    case Billboard::J3D_Y:
      m_jointData.bbMtxType = librii::j3d::MatrixType::BillboardY;
      return;
    default:
      break;
    }
  }
  u64 getNumDisplays() const override { return m_jointData.displays.size(); }
  lib3d::Bone::Display getDisplay(u64 idx) const override {
    return {(u32)m_jointData.displays[idx].material,
            (u32)m_jointData.displays[idx].shape};
  }
  void setDisplay(u64 idx, const lib3d::Bone::Display& d) override {
    auto& dst = m_jointData.displays[idx];
    dst.material = d.matId;
    dst.shape = d.polyId;
  }

  void addDisplay(const lib3d::Bone::Display& d) override {
    m_jointData.displays.emplace_back(d.matId, d.polyId);
  }

  void decompile(const librii::j3d::JointData& b) {
    m_jointData = b;
    j_flag = m_jointData.flag;
    j_bbMtxType = m_jointData.bbMtxType;
    j_mayaSSC = m_jointData.mayaSSC;
    j_boundingSphereRadius = m_jointData.boundingSphereRadius;
    j_boundingBox = m_jointData.boundingBox;
  }
  const librii::j3d::JointData compile() const {
    m_jointData.flag = j_flag;
    m_jointData.bbMtxType = j_bbMtxType;
    m_jointData.mayaSSC = j_mayaSSC;
    m_jointData.boundingSphereRadius = j_boundingSphereRadius;
    m_jointData.boundingBox = j_boundingBox;
    return m_jointData;
  }

  u16 j_flag = 1; // Unused four bits; default value in galaxy is 1
  librii::j3d::MatrixType j_bbMtxType = librii::j3d::MatrixType::Standard;
  bool j_mayaSSC = false;
  f32 j_boundingSphereRadius = 100000.0f;
  librii::math::AABB j_boundingBox{{-100000.0f, -100000.0f, -100000.0f},
                                   {100000.0f, 100000.0f, 100000.0f}};
  bool operator==(const Joint& rhs) const {
    // We treat the JointData parent as a mutable member
    compile();
    return m_jointData == rhs.m_jointData;
  }

private:
  mutable librii::j3d::JointData m_jointData{};
};

} // namespace riistudio::j3d
