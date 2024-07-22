#pragma once

#include <librii/g3d/data/AnimData.hpp>
#include <librii/g3d/io/CommonIO.hpp>
#include <librii/g3d/io/DictWriteIO.hpp>
#include <librii/g3d/io/NameTableIO.hpp>
#include <map>
#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>
#include <rsl/SafeReader.hpp>
#include <string>
#include <variant>
#include <vector>

namespace librii::g3d {

struct CLR0KeyFrame {
  u32 data{};
  bool operator==(const CLR0KeyFrame&) const = default;
};

struct CLR0Track {
  // Relative to start frame, though that seems to always be 0?
  // Also there will be numFrames + 1 keyframes for some reason, no idea why
  std::vector<CLR0KeyFrame> keyframes;

  bool operator==(const CLR0Track&) const = default;
};

struct ClrTarget {
  // final = (final & notAnimatedMask) | (animated & ~notAnimatedMask)
  u32 notAnimatedMask{};
  u32 data; // index to CLR0Track

  bool operator==(const ClrTarget&) const = default;
};

struct ClrMaterial {
  std::string name;
  u32 flags{};

  std::vector<ClrTarget> targets; // Max 11 tracks - for each channel
  bool operator==(const ClrMaterial&) const = default;
};

struct ClrAnim {
  std::vector<ClrMaterial> materials;
  std::vector<CLR0Track> tracks;
  std::string name;
  std::string sourcePath;
  u16 frameDuration{};
  AnimationWrapMode wrapMode{AnimationWrapMode::Repeat};

  Result<void> read(oishii::BinaryReader& reader);
  void write(oishii::Writer& writer, NameTable& names, u32 addrBrres) const;

  bool operator==(const ClrAnim&) const = default;
};

} // namespace librii::g3d
