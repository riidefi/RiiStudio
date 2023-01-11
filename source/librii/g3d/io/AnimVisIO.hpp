#pragma once

#include <core/common.h>
#include <librii/g3d/data/AnimData.hpp>
#include <librii/g3d/io/CommonIO.hpp>
#include <librii/g3d/io/DictIO.hpp>
#include <librii/g3d/io/DictWriteIO.hpp>
#include <librii/g3d/io/NameTableIO.hpp>
#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>
#include <rsl/SafeReader.hpp>

namespace librii::g3d {

// 8 frames of boolean animation
struct VIS0KeyFrame {
  u32 data{};
  bool operator==(const VIS0KeyFrame&) const = default;
};

struct VIS0Track {
  // Relative to start frame, though that seems to always be 0?
  // Also there will be numFrames + 1 keyframes for some reason, no idea why
  std::vector<VIS0KeyFrame> keyframes;

  bool operator==(const VIS0Track&) const = default;

  Result<void> read(rsl::SafeReader& reader, u32 numFrames) {
    for (u32 i = 0; i < numFrames; ++i) {
      keyframes.push_back(VIS0KeyFrame{.data = TRY(reader.U32())});
    }
    return {};
  }
  void write(oishii::Writer& writer) const {
    for (auto data : keyframes) {
      writer.write<u32>(data.data);
    }
  }
};

// index to CLR0Track. Just let keyFrameCount == 1 when constant
using VIS0Target = u32;

struct VIS0Bone {
  enum Flag {
    FLAG_CONSTANT_IS_VISIBLE = (1 << 0),
    FLAG_CONSTANT = (1 << 1),
  };
  std::string name;
  u32 flags{};

  std::optional<VIS0Track> target; // 1 track, just for this bone

  bool operator==(const VIS0Bone&) const = default;

  // realNumKeyFrames = roundUp(header.numFrames + 1, 32) // 32
  Result<void> read(rsl::SafeReader& reader, u32 realNumKeyFrames) {
    auto start = reader.tell();
    name = TRY(reader.StringOfs(start));
    flags = TRY(reader.U32());
    if (flags & (FLAG_CONSTANT << (0 * 2))) {
      // Value is not stored, refer to FLAG_CONSTANT_IS_VISIBLE
    } else {
      // Rather than specifying a track offset, it's simply inlined in VIS0. No
      // clue why. Super inconsistent.
      VIS0Track track;
      TRY(track.read(reader, realNumKeyFrames));
      target.emplace(track);
    }
    return {};
  }
  void write(oishii::Writer& writer, NameTable& names) const {
    auto start = writer.tell();
    writeNameForward(names, writer, start, name, true);
    writer.write<u32>(flags);
    if (target.has_value()) {
      // Rather than specifying a track offset, it's simply inlined in VIS0. No
      // clue why. Super inconsistent.
      target->write(writer);
    }
  }
};

struct BinaryVis {
  std::vector<VIS0Bone> bones;
  // Tracks are inlined in VIS0. I don't know why.

  // TODO: User data
  std::string name;
  std::string sourcePath;
  u16 frameDuration{};
  AnimationWrapMode wrapMode{AnimationWrapMode::Repeat};

  bool operator==(const BinaryVis&) const = default;

  Result<void> read(oishii::BinaryReader& reader);
  void write(oishii::Writer& writer, NameTable& names, u32 addrBrres) const;
};

} // namespace librii::g3d
