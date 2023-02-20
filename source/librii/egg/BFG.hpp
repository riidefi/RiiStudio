#pragma once

#include <core/common.h>
#include <rsl/SafeReader.hpp>

// NOTE: .bfg is not actually part of EGG; EGG instead provides a .pfog file.
namespace librii::egg {

struct BFG {
  struct Entry {
    u32 mType = 0; // librii::gx::FogType mType = librii::gx::FogType::None;
    f32 mStartZ = 0.0f;
    f32 mEndZ = 0.0f;
    u32 mColor = 0xFF;
    u16 mEnabled = 0;
    u16 mCenter = 0;
    f32 mFadeSpeed = 0.0f;
    u16 mUnk18 = 0;
    u16 mUnk1A = 0;

    bool operator==(const Entry&) const = default;
  };
  // Always 4?
  std::vector<Entry> entries;
  bool operator==(const BFG&) const = default;
};

Result<BFG> BFG_Read(rsl::SafeReader& reader);
void BFG_Write(oishii::Writer& writer, const BFG& bfg);

} // namespace librii::egg
