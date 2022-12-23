#pragma once

#include <format>
#include <librii/g3d/data/AnimData.hpp>
#include <librii/g3d/io/CommonIO.hpp>
#include <librii/g3d/io/DictIO.hpp>
#include <librii/g3d/io/DictWriteIO.hpp>
#include <librii/g3d/io/NameTableIO.hpp>
#include <map>
#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>
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

  void read(oishii::BinaryReader& reader, u32 numFrames) {
    for (u32 i = 0; i < numFrames; ++i) {
      keyframes.push_back(CLR0KeyFrame{.data = reader.read<u32>()});
    }
  }
  void write(oishii::Writer& writer) const {
    for (auto data : keyframes) {
      writer.write<u32>(data.data);
    }
  }
};

struct CLR0Target {
  // final = (final & notAnimatedMask) | (animated & ~notAnimatedMask)
  u32 notAnimatedMask{};
  std::variant<CLR0KeyFrame, u32> data; // index to CLR0Track

  bool operator==(const CLR0Target&) const = default;
};

struct CLR0Material {
  enum Flag {
    FLAG_ENABLED = (1 << 0),
    FLAG_CONSTANT = (1 << 1),
  };
  enum class TargetId {
    Material0,
    Material1,
    Ambient0,
    Ambient1,
    TevReg0, // HW + 1, skip CPREV/APREV
    TevReg1,
    TevReg2,
    KColor0,
    KColor1,
    KColor2,
    KColor3,
    Count,
  };
  std::string name;
  u32 flags{};

  std::vector<CLR0Target> targets; // Max 11 tracks - for each channel

  bool operator==(const CLR0Material&) const = default;

  void read(oishii::BinaryReader& reader, auto&& trackAddressToIndex) {
    auto start = reader.tell();
    name = readName(reader, start);
    flags = reader.read<u32>();
    u32 num_targets = 0;
    for (u32 i = 0; i < static_cast<u32>(TargetId::Count); ++i) {
      if (flags & (FLAG_ENABLED << (i * 2))) {
        ++num_targets;
      }
    }
    for (u32 i = 0; i < num_targets; ++i) {
      u32 mask = reader.read<u32>();
      if (flags & (FLAG_CONSTANT << (i * 2))) {
        CLR0KeyFrame constFrame;
        constFrame.data = reader.read<u32>();
        targets.emplace_back(
            CLR0Target{.notAnimatedMask = mask, .data = constFrame});
      } else {
        auto ofs = reader.read<s32>();
        // For some reason it's relative to the array entry? In PAT0 it's
        // relative to section start. Suppose IndexedArray and Indexed<Custom>
        // are implemented differently.
        u32 index = trackAddressToIndex(reader.tell() + ofs - 4);
        targets.emplace_back(
            CLR0Target{.notAnimatedMask = mask, .data = index});
      }
    }
  }
  void write(oishii::Writer& writer, NameTable& names,
             auto&& trackIndexToAddress) const {
    auto start = writer.tell();
    writeNameForward(names, writer, start, name, true);
    writer.write<u32>(flags);
    for (auto& target : targets) {
      writer.write<u32>(target.notAnimatedMask);
      if (auto* c = std::get_if<CLR0KeyFrame>(&target.data)) {
        writer.write<u32>(c->data);
      } else if (auto* index = std::get_if<u32>(&target.data)) {
        u32 addr = trackIndexToAddress(*index);
        // For some reason it's relative to the array entry? In PAT0 it's
        // relative to section start. Suppose IndexedArray and Indexed<Custom>
        // are implemented differently.
        auto ofs = addr - writer.tell();
        writer.write<s32>(ofs);
      } else {
        assert(!"Internal error");
      }
    }
  }
};

struct BinaryClr {
  std::vector<CLR0Material> materials;
  std::vector<CLR0Track> tracks;
  // TODO: User data
  std::string name;
  std::string sourcePath;
  u16 frameDuration{};
  AnimationWrapMode wrapMode{AnimationWrapMode::Repeat};

  bool operator==(const BinaryClr&) const = default;

  void read(oishii::BinaryReader& reader);
  void write(oishii::Writer& writer, NameTable& names, u32 addrBrres) const;
};

} // namespace librii::g3d
