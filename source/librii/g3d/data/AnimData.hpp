#pragma once

#include <array>
#include <core/common.h>
#include <optional>
#include <rsl/SmallVector.hpp>
#include <rsl/TaggedUnion.hpp>

namespace librii::g3d {

enum class AnimationWrapMode {
  Clamp,  // One-shot
  Repeat, // Loop
};

struct KeyFrame {
  f32 frame;
  f32 value;
  f32 slope;

  bool operator==(const KeyFrame&) const = default;
};

struct KeyFrameCollection {
  rsl::small_vector<KeyFrame, 16> data;
  f32 step;

  bool operator==(const KeyFrameCollection&) const = default;
};

inline f32 CalcStep(const KeyFrameCollection& collection) {
  if (collection.data.size() < 2) {
    return 0.0f;
  }

  const float first_frame = collection.data.front().frame;
  const float last_frame = collection.data.back().frame;

  if (first_frame == last_frame) {
    return 0.0f;
  }

  return 1.0f / (last_frame - first_frame);
}

enum class StorageFormat { Animated, Fixed };

struct FixedAnimation
    : public rsl::kawaiiTag<StorageFormat, StorageFormat::Fixed> {
  float value = 0.0f;

  bool operator==(const FixedAnimation&) const = default;
};

struct AnimatedAnimation
    : public rsl::kawaiiTag<StorageFormat, StorageFormat::Animated> {
  KeyFrameCollection keys;

  bool operator==(const AnimatedAnimation&) const = default;
};

struct Animation : public rsl::kawaiiUnion<StorageFormat, FixedAnimation,
                                           AnimatedAnimation> {
  Animation() : kawaiiUnion(FixedAnimation{.value = 0.0f}) {}
  using kawaiiUnion::kawaiiUnion;

  bool operator==(const Animation&) const = default;
};

// SRT

enum class SrtAttribute {
  ScaleU,
  ScaleV,
  Rotate,
  TransU,
  TransV,

  _Max
};

constexpr size_t NumberOfSrtAttributes =
    static_cast<size_t>(SrtAttribute::_Max);

struct SrtFlags {
  bool Unknown : 1 = false;

  bool ScaleOne : 1 = true;
  bool RotZero : 1 = true;
  bool TransZero : 1 = true;
  bool ScaleUniform : 1 = true;

  bool operator==(const SrtFlags&) const = default;
};

constexpr bool IsSrtAttributeIncluded(const SrtFlags& flags,
                                      SrtAttribute attribute) {
  switch (attribute) {
  case SrtAttribute::ScaleU:
    return !flags.ScaleOne;
  case SrtAttribute::ScaleV:
    return !flags.ScaleUniform;
  case SrtAttribute::Rotate:
    return !flags.RotZero;
  case SrtAttribute::TransU:
  case SrtAttribute::TransV:
    return !flags.TransZero;
  default: // _Max
    return false;
  }
}

struct SrtMatrixAnimation {
  SrtFlags flags;

  std::array<Animation, NumberOfSrtAttributes> animations;

  bool operator==(const SrtMatrixAnimation&) const = default;
};

struct SrtMaterialAnimation {
  std::string material_name = "untitled_material";

  std::array<std::optional<SrtMatrixAnimation>, 8> texture_srt;
  std::array<std::optional<SrtMatrixAnimation>, 3> indirect_srt;

  bool operator==(const SrtMaterialAnimation&) const = default;
};

struct SrtAnimationArchive {
  std::string name;
  std::string source;

  u16 frame_duration = 300;
  u32 xform_model = 0;
  AnimationWrapMode anim_wrap_mode = AnimationWrapMode::Repeat;

  rsl::small_vector<SrtMaterialAnimation, 3> mat_animations;

  bool operator==(const SrtAnimationArchive&) const = default;
};

} // namespace librii::g3d
