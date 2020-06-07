#pragma once

#include <vendor/glm/vec3.hpp>

namespace riistudio::lib3d {

//! Axis-aligned bounding box
//!
struct AABB {
  inline void expandBound(const AABB& other) {
    if (other.min.x < min.x)
      min.x = other.min.x;
    if (other.min.y < min.y)
      min.y = other.min.y;
    if (other.min.z < min.z)
      min.z = other.min.z;
    if (other.max.x > max.x)
      max.x = other.max.x;
    if (other.max.y > max.y)
      max.y = other.max.y;
    if (other.max.z > max.z)
      max.z = other.max.z;
  }

  bool operator==(const AABB& rhs) const {
    return min == rhs.min && max == rhs.max;
  }

  glm::vec3 min;
  glm::vec3 max;
};

} // namespace riistudio::lib3d
