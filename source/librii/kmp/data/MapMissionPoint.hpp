#pragma once

#include <core/common.h> // u16
#include <glm/vec3.hpp>  // glm::vec3

namespace librii::kmp {

class MissionPoint {
public:
  bool operator==(const MissionPoint&) const = default;

  glm::vec3 position;
  glm::vec3 rotation;
  u16 id;
  u16 unknown;
};

} // namespace librii::kmp
