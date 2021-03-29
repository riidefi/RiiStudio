#pragma once

#include <array>
#include <core/common.h>
#include <glm/vec3.hpp>
#include <librii/gx.h>

namespace librii::j3d {

// FIXME: This will exist in our scene and be referenced.
// For now, for 1:1, just placed here..
struct Fog {
  enum class Type {
    None,

    PerspectiveLinear,
    PerspectiveExponential,
    PerspectiveQuadratic,
    PerspectiveInverseExponential,
    PerspectiveInverseQuadratic,

    OrthographicLinear,
    OrthographicExponential,
    OrthographicQuadratic,
    OrthographicInverseExponential,
    OrthographicInverseQuadratic
  };
  Type type = Type::None;
  bool enabled = false;
  u16 center;
  f32 startZ, endZ = 0.0f;
  f32 nearZ, farZ = 0.0f;
  librii::gx::Color color = librii::gx::Color(0xffffffff);
  std::array<u16, 10> rangeAdjTable;

  bool operator==(const Fog& rhs) const noexcept = default;
};

struct NBTScale {
  bool enable = false;
  glm::vec3 scale = {0.0f, 0.0f, 0.0f};

  bool operator==(const NBTScale& rhs) const noexcept = default;
};

} // namespace librii::j3d
