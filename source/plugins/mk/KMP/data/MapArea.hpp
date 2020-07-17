#pragma once

#include <algorithm>          // std::max_element
#include <array>              // std::array
#include <core/common.h>      // u32
#include <core/kpi/Node2.hpp> // kpi::IObject
#include <glm/vec3.hpp>       // glm::vec3

namespace riistudio::mk {

class CourseMap;

enum class AreaShape {
  Box,     //!< Rectangular prism area
  Cylinder //!< Cylinder area
};

enum class AreaType {
  //! Defines the active camera within the area.
  Camera,
  //! Controls presence of various environmental particle effect systems.
  EffectController,
  //! Controls the current scene mist preset.
  FogController,
  //! Marks a pull attractor for drivers.
  PullController,
  //! Used to ground enemy path calculation in reaction to AIFall events.
  EnemyFall,
  //! Defines the orthographic capture region for the minimap model.
  MapArea2D,
  //! Enables various audio effects such as reverb and biquad coefficient
  //! presets.
  SoundController,
  //! Controls presence of Boos.
  TeresaController,
  //! Tags objects in one of 16 layers.
  ObjClipClassifier,
  //! Controls the presence of clip layers.
  ObjClipDiscriminator,
  //! Drivers will respawn when they enter this area.
  PlayerBoundary
};

// Union with children
class AreaModel {
  friend class KMP;

public:
  // Default box
  AreaModel() = default;
  AreaModel(AreaShape ashape) : mShape(ashape) {}
  ~AreaModel() = default;

  bool operator==(const AreaModel&) const = default;

  AreaShape getShape() const { return mShape; }

  const glm::vec3& getPosition() { return mPosition; }
  const glm::vec3& setPosition(const glm::vec3& pos) { return mPosition = pos; }
  const glm::vec3& getRotation() { return mRotation; }
  const glm::vec3& setRotation(const glm::vec3& rot) { return mRotation = rot; }
  const glm::vec3& getScaling() { return mScaling; }
  const glm::vec3& setScaling(const glm::vec3& scl) { return mScaling = scl; }

protected:
  AreaShape mShape = AreaShape::Box;

  // TRS block
  glm::vec3 mPosition{0.0f, 0.0f, 0.0f};
  glm::vec3 mRotation{0.0f, 0.0f, 0.0f}; // Radians
  glm::vec3 mScaling{1.0f, 1.0f, 1.0f};
};

class AreaBoxModel : public AreaModel {
  AreaBoxModel() : AreaModel(AreaShape::Box) {}
  ~AreaBoxModel() = default;

  static bool classof(const AreaModel* a) {
    return a->getShape() == AreaShape::Box;
  }
};

class AreaCylinderModel : public AreaModel {
  AreaCylinderModel() : AreaModel(AreaShape::Cylinder) {}
  ~AreaCylinderModel() = default;

  static bool classof(const AreaModel* a) {
    return a->getShape() == AreaShape::Cylinder;
  }
};

// Union with children
class Area : public virtual kpi::IObject {
  friend class KMP;

public:
  // Default camera
  Area() = default;
  Area(AreaType atype) : mType(atype) {}
  ~Area() = default;

  bool operator==(const Area& rhs) const {
    return mType == rhs.mType && mModel == rhs.mModel &&
           mCameraIndex == rhs.mCameraIndex && mPriority == rhs.mPriority &&
           mParameters == rhs.mParameters && mRailID == rhs.mRailID &&
           mEnemyLinkID == rhs.mEnemyLinkID && mPad == rhs.mPad;
  }

  AreaType getType() const { return mType; }

  //! Get the intersection model of this area;
  AreaModel& getModel() { return mModel; }
  //! Get the intersection model of this area;
  const AreaModel& getModel() const { return mModel; }

  // Lower is higher
  u8 getPriority() const { return 0xFF - mPriority; }
  void setPriority(u8 p) { mPriority = 0xFF - p; }

  std::string getName() const override {
    constexpr std::array<const char*, 11> areaTypes{"Camera Area",
                                                    "EffectController Area",
                                                    "FogController Area",
                                                    "PullController Area",
                                                    "EnemyFall Area",
                                                    "MapArea2D Area",
                                                    "SoundController Area",
                                                    "TeresaController Area",
                                                    "ObjClipClassifier Area",
                                                    "ObjClipDiscriminator Area",
                                                    "PlayerBoundary Area"};
    return areaTypes[static_cast<int>(getType())];
  }

protected:
  AreaType mType = AreaType::Camera;
  AreaModel mModel;

  s32 mCameraIndex = -1; // Valid for AreaType::Camera
  u8 mPriority = 0;      // Greater -> higher priority

  std::array<u16, 2> mParameters{0, 0};

  // R2200:

  u32 mRailID = 0;
  u32 mEnemyLinkID = 0;
  std::array<u8, 2> mPad{0, 0};
};

class CameraArea : public Area {
public:
  CameraArea() : Area(AreaType::Camera) {}

  s32 getCameraIndex() const { return mCameraIndex; }
  void setCameraIndex(s32 idx) { mCameraIndex = idx; }

  static bool classof(const Area* a) {
    return a->getType() == AreaType::Camera;
  }
};

class EffectArea : public Area {
public:
  EffectArea() : Area(AreaType::EffectController) {}

  u16 getEffectType() const { return mParameters[0]; }
  void setEffectType(u16 etype) { mParameters[0] = etype; }

  static bool classof(const Area* a) {
    return a->getType() == AreaType::EffectController;
  }
};

class FogArea : public Area {
public:
  FogArea() : Area(AreaType::FogController) {}

  u16 getFogIndex() const { return mParameters[0]; }
  void setFogIndex(u16 idx) { mParameters[0] = idx; }

  static bool classof(const Area* a) {
    return a->getType() == AreaType::FogController;
  }
};

class PullArea : public Area {
public:
  PullArea() : Area(AreaType::PullController) {}

  u16 getParam1() const { return mParameters[0]; }
  void getParam1(u16 idx) { mParameters[0] = idx; }

  u16 getParam2() const { return mParameters[1]; }
  void getParam2(u16 idx) { mParameters[1] = idx; }

  u8 getRailId() const { return mRailID; }
  void setRailId(u8 id) { mRailID = id; }

  static bool classof(const Area* a) {
    return a->getType() == AreaType::PullController;
  }
};

class EnemyFallArea : public Area {
public:
  EnemyFallArea() : Area(AreaType::EnemyFall) {}

  u8 getEnemyLinkId() const { return mEnemyLinkID; }
  void setEnemyLinkId(u8 id) { mEnemyLinkID = id; }

  static bool classof(const Area* a) {
    return a->getType() == AreaType::EnemyFall;
  }
};

class Map2DArea : public Area {
public:
  Map2DArea() : Area(AreaType::MapArea2D) {}

  static bool classof(const Area* a) {
    return a->getType() == AreaType::MapArea2D;
  }
};

class SoundArea : public Area {
public:
  SoundArea() : Area(AreaType::SoundController) {}

  u16 getParam1() const { return mParameters[0]; }
  void getParam1(u16 idx) { mParameters[0] = idx; }

  u16 getParam2() const { return mParameters[1]; }
  void getParam2(u16 idx) { mParameters[1] = idx; }

  static bool classof(const Area* a) {
    return a->getType() == AreaType::SoundController;
  }
};

class ObjClipArea : public Area {
public:
  ObjClipArea(AreaType a) : Area(a) {}

  u16 getGroupId() const { return mParameters[0]; }
  void setGroupId(u16 idx) { mParameters[0] = idx; }

  static bool classof(const Area* a) {
    return a->getType() == AreaType::ObjClipClassifier ||
           a->getType() == AreaType::ObjClipDiscriminator;
  }
};

class ObjClipClassifierArea : public ObjClipArea {
public:
  ObjClipClassifierArea() : ObjClipArea(AreaType::ObjClipClassifier) {}

  static bool classof(const Area* a) {
    return a->getType() == AreaType::ObjClipClassifier;
  }
};

class ObjClipDiscriminatorArea : public ObjClipArea {
public:
  ObjClipDiscriminatorArea() : ObjClipArea(AreaType::ObjClipDiscriminator) {}

  static bool classof(const Area* a) {
    return a->getType() == AreaType::ObjClipDiscriminator;
  }
};

class BoundaryArea : public Area {
public:
  BoundaryArea() : Area(AreaType::PlayerBoundary) {}

  enum class ConstraintType {
    Whitelist, //!< The area is enabled within the range
    Blacklist  //!< The area is disabled within the range
  };

  //! Return if the area's function as a fall boundary is subject to a
  //! constraint.
  bool isConstrained() const { return mParameters[0] != mParameters[1]; }
  //! Unconditionally enable the area.
  void forgetConstraint() { mParameters[1] = mParameters[0]; }
  //!
  void defaultConstraint() { mParameters[1] = mParameters[0] + 1; }

  //! Get the constraint type.
  //! @pre Operation is undefined when `isConstrained()` is not true.
  ConstraintType getConstraintType() const {
    return mParameters[1] > mParameters[0] ? ConstraintType::Whitelist
                                           : ConstraintType::Blacklist;
  }
  //! Set the constaint type.
  //! @pre Operation is void when unconstrained.
  void setConstraintType(ConstraintType c) {
    if (getConstraintType() != c)
      std::swap(mParameters[0], mParameters[1]);
  }

  //! Return the minimum checkpoint index for the condition to be met.
  u16 getInclusiveLowerBound() const {
    return std::min(mParameters[0], mParameters[1]);
  }
  //! Set the minimum checkpoint index for the condition to be met.
  void setInclusiveLowerBound(u16 m) {
    *std::min_element(mParameters.begin(), mParameters.end()) = m;
  }

  //! Return the maximum checkpoint index for the condition to be met.
  u16 getInclusiveUpperBound() const {
    return std::max(mParameters[0], mParameters[1]) - 1;
  }
  //! Set the maximum checkpoint index for the condition to be met.
  void setInclusiveUpperBound(u16 m) {
    *std::max_element(mParameters.begin(), mParameters.end()) = m + 1;
  }

  // Discriminator for llvm::dyn_cast, llvm::is_a, etc.
  static bool classof(const Area* a) {
    return a->getType() == AreaType::PlayerBoundary;
  }
};

} // namespace riistudio::mk
