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
};

} // namespace librii::kmp
