#pragma once

#include <algorithm>     // std::max_element
#include <array>         // std::array
#include <core/common.h> // u32
#include <glm/vec3.hpp>  // glm::vec3

namespace librii::kmp {

enum class AreaShape {
  Box,      //!< Rectangular prism area
  Cylinder, //!< Cylinder area
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
  BloomController,
  //! Controls presence of Boos.
  TeresaController,
  //! Tags objects in one of 16 layers.
  ObjClipClassifier,
  //! Controls the presence of clip layers.
  ObjClipDiscriminator,
  //! Drivers will respawn when they enter this area.
  PlayerBoundary,
};

// Union with children
struct AreaModel {
  AreaShape mShape = AreaShape::Box;

  // TRS block
  glm::vec3 mPosition{0.0f, 0.0f, 0.0f};
  glm::vec3 mRotation{0.0f, 0.0f, 0.0f}; // Radians
  glm::vec3 mScaling{1.0f, 1.0f, 1.0f};

  bool operator==(const AreaModel&) const = default;
  AreaShape getShape() const { return mShape; }
};

struct Area {
  AreaType mType = AreaType::Camera;
  AreaModel mModel;
  s32 mCameraIndex = -1; // Valid for AreaType::Camera
  u8 mPriority = 0;      // Greater -> higher priority
  std::array<u16, 2> mParameters{0, 0};
  // R2200:
  u32 mRailID = 0;
  u32 mEnemyLinkID = 0;
  std::array<u8, 2> mPad{0, 0};

  bool operator==(const Area&) const = default;
  AreaType getType() const { return mType; }

  //! Get the intersection model of this area;
  AreaModel& getModel() { return mModel; }
  //! Get the intersection model of this area;
  const AreaModel& getModel() const { return mModel; }
};

class BoundaryArea : public Area {
public:
  BoundaryArea() { mType = AreaType::PlayerBoundary; }

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

} // namespace librii::kmp
