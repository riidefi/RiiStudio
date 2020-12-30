#pragma once

#include <core/common.h>
#include <plugins/gc/Export/Material.hpp>

namespace riistudio::mk {

enum class FogType {
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

class FogEntry {

public:
  bool operator==(const FogEntry&) const = default;

  FogType mType;
  f32 mStartZ;
  f32 mEndZ;
  librii::gx::Color mColor;
  u16 mEnabled;
  u16 mCenter;
  f32 mFadeSpeed;
  u16 mUnk18;
  u16 mUnk1A;
};

} // namespace riistudio::mk
