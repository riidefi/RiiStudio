#pragma once

#include <core/common.h> // u16
#include <rsl/Types.hpp>

namespace librii::kmp {

struct RespawnPoint {
  bool operator==(const RespawnPoint&) const = default;

  glm::vec3 position;
  glm::vec3 rotation;
  u16 id{};
  s16 range{};
};

} // namespace librii::kmp
