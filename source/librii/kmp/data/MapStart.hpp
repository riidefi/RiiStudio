#pragma once

#include <glm/vec3.hpp> // glm::vec3
#include <rsl/Types.hpp>

namespace librii::kmp {

struct StartPoint {
  glm::vec3 position;
  glm::vec3 rotation;
  s16 player_index{};
  u16 _{};

  bool operator==(const StartPoint&) const = default;
};

} // namespace librii::kmp
