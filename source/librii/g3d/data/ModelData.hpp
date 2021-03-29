#pragma once

#include <librii/math/aabb.hpp>
#include <string>

namespace librii::g3d {

enum class ScalingRule { Standard, XSI, Maya };
enum class TextureMatrixMode { Maya, XSI, Max };
enum class EnvelopeMatrixMode { Normal, Approximation, Precision };

struct G3DModelDataData {
  ScalingRule mScalingRule = ScalingRule::Standard;
  TextureMatrixMode mTexMtxMode = TextureMatrixMode::Maya;
  EnvelopeMatrixMode mEvpMtxMode = EnvelopeMatrixMode::Normal;
  std::string sourceLocation;
  librii::math::AABB aabb;

  std::string mName = "course";

  bool operator==(const G3DModelDataData& rhs) const = default;
};

} // namespace librii::g3d
