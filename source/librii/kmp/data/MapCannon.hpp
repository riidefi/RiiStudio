#pragma once

#include <array>        // std::array
#include <glm/vec3.hpp> // glm::vec3
#include <rsl/Types.hpp>

namespace librii::kmp {

enum class CannonType {
  Direct,
  Curved,
  Curved2,

  // < 0 => Direct
  Default = -1,
};

struct Cannon {
  CannonType mType = CannonType::Direct;
  glm::vec3 mPosition{0.0f, 0.0f, 0.0f};
  glm::vec3 mRotation{0.0f, 0.0f, 0.0f};

  bool operator==(const Cannon&) const = default;
};

} // namespace librii::kmp
