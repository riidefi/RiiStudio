#pragma once

#include <core/common.h>

#include <oishii/writer/binary_writer.hxx>
#include <rsl/SafeReader.hpp>

#include "AnimIO.hpp"

namespace librii::g3d {

enum class ChrQuantization {
  Track32,
  Track48,
  Track96,
  BakedTrack8,
  BakedTrack16,
  BakedTrack32,
  Const,
};

struct ChrFrame {
  // We actually need 64 bits of precision currently to get a match. We should
  // be able to finesse byte-matching output while keeping f32 output given the
  // data was originally 32-bit precision?
  f64 frame, value, slope;
  bool operator==(const ChrFrame&) const = default;
};

struct ChrTrack {
  ChrQuantization quant{ChrQuantization::Track96};
  // These are only quantization settings. Do not consider scale/offset when
  // interpreting `frames`
  float scale{1.0f};
  float offset{0.0f};

  // Cached value for BrawlBox compatibility
  float step{0.0f};

  std::vector<ChrFrame> frames;

  bool operator==(const ChrTrack&) const = default;
};

struct ChrNode {
  std::string name;
  u32 flags;
  // All indices
  std::vector<u32> tracks;

  bool operator==(const ChrNode&) const = default;
};

struct ChrAnim {
  std::vector<ChrNode> nodes;
  // All existing tracks + extra tracks (to be trimmed) for Const values.
  std::vector<ChrTrack> tracks;
  // TODO: User data
  std::string name;
  std::string sourcePath;
  u16 frameDuration{};
  AnimationWrapMode wrapMode{AnimationWrapMode::Repeat};
  u32 scaleRule = 0;

  bool operator==(const ChrAnim&) const = default;
};

} // namespace librii::g3d
