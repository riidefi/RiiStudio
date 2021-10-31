#pragma once

#include <core/common.h> // u16
#include <glm/vec3.hpp>  // glm::vec3

namespace librii::kmp {

class StartPoint {
public:
  bool operator==(const StartPoint&) const = default;

  glm::vec3 position;
  glm::vec3 rotation;
  s16 player_index = 0;
  s16 _ = 0;
};

} // namespace librii::kmp
