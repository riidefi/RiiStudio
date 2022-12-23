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

struct PAT0KeyFrame {
  u16 texture{};
  u16 palette{};

  bool operator==(const PAT0KeyFrame&) const = default;

  void read(oishii::BinaryReader& reader) {
    texture = reader.read<u16>();
    palette = reader.read<u16>();
  }
  void write(oishii::Writer& writer) const {
    writer.write<u16>(texture);
    writer.write<u16>(palette);
  }
};
struct PAT0Track {
  std::map<f32, PAT0KeyFrame>
      keyframes; // frame index is a float for some reason
  u16 reserved{};
  f32 progressPerFrame{};

  bool operator==(const PAT0Track&) const = default;

  void read(oishii::BinaryReader& reader) {
    auto count = reader.read<u16>();
    reserved = reader.read<u16>();
    progressPerFrame = reader.read<f32>();
    for (u16 i = 0; i < count; ++i) {
      auto frame = reader.read<f32>();
      PAT0KeyFrame data;
      data.read(reader);
      keyframes.emplace(frame, data);
    }
  }
  void write(oishii::Writer& writer) const {
    writer.write<u16>(keyframes.size());
    writer.write<u16>(reserved);
    writer.write<f32>(progressPerFrame);
    for (auto& [frame, data] : keyframes) {
      writer.write<f32>(frame);
      data.write(writer);
    }
  }
};

using PAT0Sampler = std::variant<PAT0KeyFrame,
                                 /* Index */
                                 u32>;

struct PAT0Material {
  enum Flag {
    FLAG_ENABLED = (1 << 0),
    FLAG_CONSTANT = (1 << 1),
    FLAG_TEXTURES = (1 << 2),
    FLAG_PALETTES = (1 << 3),
  };
  std::string name;
  u32 flags{};
  std::vector<PAT0Sampler> samplers; // Max 8 tracks - for each texmap

  bool operator==(const PAT0Material&) const = default;

  void read(oishii::BinaryReader& reader, auto&& trackAddressToIndex) {
    auto start = reader.tell();
    name = readName(reader, start);
    flags = reader.read<u32>();
    u32 num_samplers = 0;
    for (u32 i = 0; i < 8; ++i) {
      if (flags & (FLAG_ENABLED << (i * 4))) {
        ++num_samplers;
      }
    }
    for (u32 i = 0; i < num_samplers; ++i) {
      if (flags & (FLAG_CONSTANT << (i * 4))) {
        PAT0KeyFrame constant;
        constant.read(reader);
        samplers.emplace_back(constant);
      } else {
        auto ofs = reader.read<s32>();
        u32 index = trackAddressToIndex(start + ofs);
        samplers.emplace_back(index);
      }
    }
  }
  void write(oishii::Writer& writer, NameTable& names,
             auto&& trackIndexToAddress) const {
    auto start = writer.tell();
    writeNameForward(names, writer, start, name, true);
    writer.write<u32>(flags);
    for (auto& sampler : samplers) {
      if (auto* c = std::get_if<PAT0KeyFrame>(&sampler)) {
        c->write(writer);
      } else if (auto* index = std::get_if<u32>(&sampler)) {
        u32 addr = trackIndexToAddress(*index);
        auto ofs = addr - start;
        writer.write<s32>(ofs);
      } else {
        assert(!"Internal error");
      }
    }
  }
};

struct BinaryTexPat {
  std::vector<PAT0Material> materials;
  std::vector<PAT0Track> tracks;
  std::vector<std::string> textureNames;
  std::vector<std::string> paletteNames;
  std::vector<u32> runtimeTextures;
  std::vector<u32> runtimePalettes;
  // TODO: User data
  std::string name;
  std::string sourcePath;
  u16 frameDuration{};
  AnimationWrapMode wrapMode{AnimationWrapMode::Repeat};

  bool operator==(const BinaryTexPat&) const = default;

  void read(oishii::BinaryReader& reader);
  void write(oishii::Writer& writer, NameTable& names, u32 addrBrres) const;
};

} // namespace librii::g3d
