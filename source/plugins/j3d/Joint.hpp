#pragma once

#include <core/common.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include <core/3d/i3dmodel.hpp>

#include <plugins/gc/Export/Bone.hpp>

namespace riistudio::j3d {

template <typename T> using ID = int;
struct Material;
struct Shape;
struct Joint;
struct JointData {
  // Four LSBs of flag; left is matrix type
  enum class MatrixType : u16 { Standard = 0, Billboard, BillboardY };

  std::string name = "root";
  ID<Joint> id = 0;

  u16 flag = 1; // Unused four bits; default value in galaxy is 1
  MatrixType bbMtxType = MatrixType::Standard;
  bool mayaSSC = false; // 0xFF acts as false -- likely for compatibility

  glm::vec3 scale{1.0f, 1.0f, 1.0f};
  glm::vec3 rotate{0.0f, 0.0f, 0.0f};
  glm::vec3 translate{0.0f, 0.0f, 0.0f};

  f32 boundingSphereRadius = 100000.0f;
  lib3d::AABB boundingBox{{-100000.0f, -100000.0f, -100000.0f},
                          {100000.0f, 100000.0f, 100000.0f}};

  // From INF1
  ID<Joint> parentId = -1;
  std::vector<ID<Joint>> children;

  struct Display {
    ID<Material> material;
    ID<Shape> shape;

    Display() = default;
    Display(ID<Material> mat, ID<Shape> shp) : material(mat), shape(shp) {}
    bool operator==(const Display& rhs) const {
      return material == rhs.material && shape == rhs.shape;
    }
  };
  std::vector<Display> displays;

  // From EVP1
  // glm::mat4x4
  std::array<float, 12> inverseBindPoseMtx;

  bool operator==(const JointData& rhs) const {
    return name == rhs.name && id == rhs.id && bbMtxType == rhs.bbMtxType &&
           mayaSSC == rhs.mayaSSC && scale == rhs.scale &&
           rotate == rhs.rotate && translate == rhs.translate &&
           boundingSphereRadius == rhs.boundingSphereRadius &&
           boundingBox == rhs.boundingBox && parentId == rhs.parentId &&
           children == rhs.children && displays == rhs.displays &&
           inverseBindPoseMtx == rhs.inverseBindPoseMtx;
  }
};
struct Joint : public libcube::IBoneDelegate, public JointData {
  // PX_TYPE_INFO_EX("J3D Joint", "j3d_joint", "J::Joint", ICON_FA_BONE,
  // ICON_FA_BONE);

  std::string getName() const { return name; }
  void setName(const std::string& n) override { name = n; }
  s64 getId() override { return id; }
  void copy(lib3d::Bone& to) const override {
    IBoneDelegate::copy(to);
    Joint* pJoint = dynamic_cast<Joint*>(&to);
    if (pJoint) {
      pJoint->name = name;
      pJoint->flag = flag;
    }
  }
  lib3d::Coverage supportsBoneFeature(lib3d::BoneFeatures f) override {
    switch (f) {
    case lib3d::BoneFeatures::SRT:
    case lib3d::BoneFeatures::Hierarchy:
    case lib3d::BoneFeatures::StandardBillboards:
    case lib3d::BoneFeatures::AABB:
    case lib3d::BoneFeatures::BoundingSphere:
    case lib3d::BoneFeatures::SegmentScaleCompensation:
      return lib3d::Coverage::ReadWrite;
    default:
      return lib3d::Coverage::Unsupported;
    }
  }

  lib3d::SRT3 getSRT() const override { return {scale, rotate, translate}; }
  void setSRT(const lib3d::SRT3& srt) override {
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
  lib3d::AABB getAABB() const override { return boundingBox; }
  void setAABB(const lib3d::AABB& aabb) override {
    boundingBox = {aabb.min, aabb.max};
  }
  float getBoundingRadius() const override { return boundingSphereRadius; }
  void setBoundingRadius(float r) override { boundingSphereRadius = r; }
  bool getSSC() const override { return mayaSSC; }
  void setSSC(bool b) override { mayaSSC = b; }

  Billboard getBillboard() const override {
    switch (bbMtxType) {
    case MatrixType::Standard:
      return Billboard::None;
    case MatrixType::Billboard:
      return Billboard::J3D_XY;
    case MatrixType::BillboardY:
      return Billboard::J3D_Y;
    default:
      return Billboard::None;
    }
  }
  void setBillboard(Billboard b) override {
    switch (b) {
    case Billboard::None:
      bbMtxType = MatrixType::Standard;
      break;
    case Billboard::J3D_XY:
      bbMtxType = MatrixType::Billboard;
      break;
    case Billboard::J3D_Y:
      bbMtxType = MatrixType::BillboardY;
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
};

} // namespace riistudio::j3d
