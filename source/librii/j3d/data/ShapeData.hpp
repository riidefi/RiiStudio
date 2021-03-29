#pragma once

#include <core/common.h>
#include <librii/math/aabb.hpp>

namespace librii::j3d {

struct ShapeData : public librii::gx::MeshData {
  u32 id;
  enum class Mode {
    Normal,
    Billboard_XY,
    Billboard_Y,
    Skinned,

    Max
  };
  Mode mode = Mode::Normal;

  f32 bsphere = 100000.0f;
  librii::math::AABB bbox{{-100000.0f, -100000.0f, -100000.0f},
                          {100000.0f, 100000.0f, 100000.0f}};

  bool operator==(const ShapeData&) const = default;
};

} // namespace librii::j3d
