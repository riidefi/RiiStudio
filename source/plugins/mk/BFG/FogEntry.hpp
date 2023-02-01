#pragma once

#include <core/common.h>
#include <librii/gx.h>
#include <plugins/gc/Export/Material.hpp>

namespace riistudio::mk {

class FogEntry {
public:
  bool operator==(const FogEntry&) const = default;

  std::string getName() const { return "TODO"; }

  librii::gx::FogType mType = librii::gx::FogType::None;
  f32 mStartZ = 0.0f;
  f32 mEndZ = 0.0f;
  librii::gx::Color mColor = {0, 0, 0, 0xFF};
  u16 mEnabled = 0;
  u16 mCenter = 0;
  f32 mFadeSpeed = 0.0f;
  u16 mUnk18 = 0;
  u16 mUnk1A = 0;
};

} // namespace riistudio::mk
