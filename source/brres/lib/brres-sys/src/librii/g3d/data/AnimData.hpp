#pragma once

#include <array>
#include <cmath> // std::nextafter
#include <core/common.h>
#include <optional>
#include <rsl/SmallVector.hpp>
#include <string>

namespace librii::g3d {

enum class AnimationWrapMode {
  Clamp,  // One-shot
  Repeat, // Loop
};

static inline float CalcStep(float begin, float end) {
  assert(begin >= 0.0f);
  assert(end >= 0.0f);
  assert(end >= begin);

  float interval = end - begin;
  while ((1.0f / interval) * (end - begin) >= 1.0f) {
    interval = std::nextafter(interval, std::numeric_limits<float>::max());
  }
  return (1.0f / interval);
}

struct SRT0KeyFrame {
  f32 frame{};
  f32 value{};
  f32 tangent{};
  bool operator==(const SRT0KeyFrame&) const = default;
};

// Binary format (.srt0)
struct BinarySrt;

struct Model;

// Abstracted structure
//
// - "Constant" animations are expressed as a single keyframe.
// - List of animated matrices is provided.
//
struct SrtAnim {
  struct Target {
    std::string materialName{};
    bool indirect = false;
    int matrixIndex = 0;
    bool operator==(const Target&) const = default;
  };

  // 1 keyframe <=> constant
  using Track = std::vector<SRT0KeyFrame>;

  struct Mtx {
    Track scaleX{};
    Track scaleY{};
    Track rot{};
    Track transX{};
    Track transY{};

    Track& subtrack(size_t idx) {
      assert(idx >= 0 && idx < 5);
      return (&scaleX)[idx];
    }
    const Track& subtrack(size_t idx) const {
      assert(idx >= 0 && idx < 5);
      return (&scaleX)[idx];
    }
    bool operator==(const Mtx&) const = default;
  };

  struct TargetedMtx {
    Target target;
    Mtx matrix;
    bool operator==(const TargetedMtx&) const = default;
  };

  std::vector<TargetedMtx> matrices{};
  std::string name{"Untitled SRT0"};
  std::string sourcePath{};
  u16 frameDuration{};
  u32 xformModel{};
  AnimationWrapMode wrapMode{AnimationWrapMode::Repeat};

  bool operator==(const SrtAnim&) const = default;

  static Result<SrtAnim> read(const BinarySrt& srt,
                              std::function<void(std::string_view)> warn);
  static BinarySrt write(const SrtAnim& anim,
                         const librii::g3d::Model* mdl = nullptr);
};

using SrtAnimationArchive = SrtAnim;

} // namespace librii::g3d
