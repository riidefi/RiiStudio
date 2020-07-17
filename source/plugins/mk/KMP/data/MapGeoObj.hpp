#pragma once

#include <array>         // std::array
#include <core/common.h> // u16
#include <glm/vec3.hpp>  // glm::vec3

namespace riistudio::mk {

//! A geography object
class GeoObj {
public:
  bool operator==(const GeoObj&) const = default;

  u16 id{0};
  u16 _{0};

  glm::vec3 position{0.0f};
  glm::vec3 rotation{0.0f};
  glm::vec3 scale{1.0f};

  s16 pathId{-1};
  std::array<u16, 8> settings;
  u16 flags;
};

} // namespace riistudio::mk
