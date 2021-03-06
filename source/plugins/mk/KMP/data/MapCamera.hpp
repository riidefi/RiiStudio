#pragma once

#include <core/common.h> // u32
#include <glm/vec3.hpp>  // glm::vec3

namespace riistudio::mk {

class CourseMap;

enum class CameraType {
  Goal,
  Fixed,
  Path,
  Follow,      // -1
  FixedMoveAt, // Opening
  PathMoveAt,  // Opening, with variant PathMoveAt2
  FollowPath,  // -1
  FollowPath2, // 1
  FollowPath3, // 2
  MissionSuccess
};

class Camera {
  friend class KMP;

public:
  bool operator==(const Camera&) const = default;

  CameraType getType() const { return mType; }

protected:
  template <typename T> struct LinearAttribute {
    u16 mSpeed = 0; // * (fovYSpeed, viewSpeed)
    T from{};
    T to{};

    bool operator==(const LinearAttribute&) const = default;
  };

  CameraType mType = CameraType::Goal;

  u8 mNext = 0xFF; // Intrusive graph, 0xFF sentinel
  u8 mShake = 0;   // *

  u8 mPathId = 0xFF;  // 0xFF to disable
  u16 mPathSpeed = 0; // *

  u8 mStartFlag = 0; // *
  u8 mMovieFlag = 0; // * In-engine recording?

  glm::vec3 mPosition{0.0f, 0.0f, 0.0f};
  glm::vec3 mRotation{0.0f, 0.0f, 0.0f};

  LinearAttribute<f32> mFov;
  LinearAttribute<glm::vec3> mView;

  f32 mActiveFrames = 0;
};

} // namespace riistudio::mk
