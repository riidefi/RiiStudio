#pragma once

#include <librii/g3d/data/AnimData.hpp>
#include <librii/g3d/io/CommonIO.hpp>
#include <librii/g3d/io/DictWriteIO.hpp>
#include <librii/g3d/io/ModelIO.hpp>
#include <librii/g3d/io/NameTableIO.hpp>
#include <map>
#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>
#include <rsl/SafeReader.hpp>
#include <string>
#include <variant>
#include <vector>

namespace librii::g3d {

struct SRT0Track {
  // There will be numFrames + 1 keyframes for some reason
  std::vector<SRT0KeyFrame> keyframes{};
  std::array<u8, 2> reserved{};
  f32 step{};

  bool operator==(const SRT0Track&) const = default;

  u32 computeSize() const;
  Result<void> read(rsl::SafeReader& safe);
  void write(oishii::Writer& writer) const;
};

struct SRT0Target {
  std::variant<f32, u32> data; // index to SRT0Track

  bool operator==(const SRT0Target&) const = default;
};

struct SRT0Matrix {
  enum Flag {
    FLAG_ENABLED = (1 << 0),
    FLAG_SCL_ONE = (1 << 1),
    FLAG_ROT_ZERO = (1 << 2),
    FLAG_TRANS_ZERO = (1 << 3),
    FLAG_SCL_ISOTROPIC = (1 << 4),
    FLAG_SCL_U_FIXED = (1 << 5),
    FLAG_SCL_V_FIXED = (1 << 6),
    FLAG_ROT_FIXED = (1 << 7),
    FLAG_TRANS_U_FIXED = (1 << 8),
    FLAG_TRANS_V_FIXED = (1 << 9),

    FLAG__COUNT = 10,
  };
  enum class TargetId {
    ScaleU,
    ScaleV,
    Rotate,
    TransU,
    TransV,
    Count,
  };
  static bool isFixed(TargetId target, u32 flags) {
    switch (target) {
    case TargetId::ScaleU:
      return flags & FLAG_SCL_U_FIXED;
    case TargetId::ScaleV:
      return flags & FLAG_SCL_V_FIXED;
    case TargetId::Rotate:
      return flags & FLAG_ROT_FIXED;
    case TargetId::TransU:
      return flags & FLAG_TRANS_U_FIXED;
    case TargetId::TransV:
      return flags & FLAG_TRANS_V_FIXED;
    case TargetId::Count:
      break; // Not a valid enum value
    }
    return false;
  }
  static bool isAttribIncluded(SRT0Matrix::TargetId attribute, u32 flags) {
    switch (attribute) {
    case TargetId::ScaleU:
      return (flags & FLAG_SCL_ONE) == 0;
    case TargetId::ScaleV:
      return (flags & FLAG_SCL_ISOTROPIC) == 0;
    case TargetId::Rotate:
      return (flags & FLAG_ROT_ZERO) == 0;
    case TargetId::TransU:
    case TargetId::TransV:
      return (flags & FLAG_TRANS_ZERO) == 0;
    case TargetId::Count:
      break; // Not a valid enum value
    }
    return false;
  }
  u32 flags{};

  std::vector<SRT0Target> targets; // Max 5 tracks - for each channel

  bool operator==(const SRT0Matrix&) const = default;

  u32 computeSize() const;
  Result<void> read(rsl::SafeReader& safe,
                    std::function<Result<u32>(u32)> trackAddressToIndex);
  void write(oishii::Writer& writer,
             std::function<u32(u32)> trackIndexToAddress) const;
};

struct SRT0Material {
  enum Flag {
    FLAG_ENABLED = (1 << 0),
  };
  std::string name{};
  u32 enabled_texsrts{};
  u32 enabled_indsrts{};
  // Max 8+3 matrices. These can't be merged and are placed
  // inline by the converter for some reason.
  std::vector<SRT0Matrix> matrices{};

  bool operator==(const SRT0Material&) const = default;

  u32 computeSize() const;
  [[nodiscard]] Result<void>
  read(rsl::SafeReader& reader,
       std::function<Result<u32>(u32)> trackAddressToIndex);
  [[nodiscard]] Result<void>
  write(oishii::Writer& writer, NameTable& names,
        std::function<u32(u32)> trackIndexToAddress) const;
};

struct BinarySrt {
  std::vector<SRT0Material> materials;
  std::vector<SRT0Track> tracks;
  // TODO: User data
  std::string name;
  std::string sourcePath;
  u16 frameDuration{};
  u32 xformModel{};
  AnimationWrapMode wrapMode{AnimationWrapMode::Repeat};

  bool operator==(const BinarySrt&) const = default;

  [[nodiscard]] Result<void> read(oishii::BinaryReader& reader);
  [[nodiscard]] Result<void> write(oishii::Writer& writer, NameTable& names,
                                   u32 addrBrres) const;

  void mergeIdenticalTracks();
};

} // namespace librii::g3d
