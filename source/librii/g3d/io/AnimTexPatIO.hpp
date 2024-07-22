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

struct PAT0KeyFrame {
  u16 texture{};
  u16 palette{};

  bool operator==(const PAT0KeyFrame&) const = default;
};
struct PAT0Track {
  std::map<f32, PAT0KeyFrame>
      keyframes; // frame index is a float for some reason
  u16 reserved{};
  f32 progressPerFrame{};

  bool operator==(const PAT0Track&) const = default;
};

struct PatMaterial {
  std::string name;
  u32 flags{};
  std::vector<u32> samplers; // Max 8 tracks - for each texmap
  bool operator==(const PatMaterial&) const = default;
};

struct PatAnim {
  std::vector<PatMaterial> materials;
  std::vector<PAT0Track> tracks;
  std::vector<std::string> textureNames;
  std::vector<std::string> paletteNames;
  std::vector<u32> runtimeTextures;
  std::vector<u32> runtimePalettes;
  std::string name;
  std::string sourcePath;
  u16 frameDuration{};
  AnimationWrapMode wrapMode{AnimationWrapMode::Repeat};

  bool operator==(const PatAnim&) const = default;
};

} // namespace librii::g3d
