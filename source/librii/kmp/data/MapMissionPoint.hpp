#pragma once

#include <glm/vec3.hpp> // glm::vec3
#include <rsl/Types.hpp>

namespace librii::kmp {

struct MissionPoint {
  glm::vec3 position;
  glm::vec3 rotation;
  u16 id{};
  u16 unknown{};

  bool operator==(const MissionPoint&) const = default;
};

} // namespace librii::kmp
