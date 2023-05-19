#pragma once

#include <librii/g3d/data/AnimData.hpp>
#include <librii/g3d/io/CommonIO.hpp>
#include <librii/g3d/io/DictIO.hpp>
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

struct SRT0KeyFrame {
  f32 frame{};
  f32 value{};
  f32 tangent{};
  bool operator==(const SRT0KeyFrame&) const = default;
};

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

// XML-suitable variant
struct SrtAnim {
  struct Target {
    std::string materialName{};
    bool indirect = false;
    int matrixIndex = 0;
    bool operator==(const Target&) const = default;
  };

  using Track = std::variant<f32, std::vector<SRT0KeyFrame>>;

  struct Mtx {
    Track scaleX{};
    Track scaleY{};
    Track rot{};
    Track transX{};
    Track transY{};
    bool operator==(const Mtx&) const = default;
  };

  struct TargetedMtx {
    Target target;
    Mtx matrix;
    bool operator==(const TargetedMtx&) const = default;
  };

  std::vector<TargetedMtx> matrices{};
  std::string name{};
  std::string sourcePath{};
  u16 frameDuration{};
  u32 xformModel{};
  AnimationWrapMode wrapMode{AnimationWrapMode::Repeat};

  bool operator==(const SrtAnim&) const = default;

  static Result<SrtAnim> read(const BinarySrt& srt,
                              std::function<void(std::string_view)> warn) {
    SrtAnim tmp{};
    tmp.name = srt.name;
    tmp.sourcePath = srt.sourcePath;
    tmp.frameDuration = srt.frameDuration;
    tmp.xformModel = srt.xformModel;
    tmp.wrapMode = srt.wrapMode;

    // Suppose our sort index array looks like
    //
    // 0  1 -1  2  3  4 -1 -1  5
    // Our sortBiasCtr and lastSort variable logic will instead produce
    // 0  1  2  3  4  5  6  7  8
    //
    std::vector<int> sortIndices;
    int sortBiasCtr = 0;
    int lastSort = 0;

    for (auto& m : srt.materials) {
      size_t j = 0;
      for (size_t i = 0; i < 8; ++i) {
        if (m.enabled_texsrts & (1 << i)) {
          TargetedMtx targetedMtx;
          targetedMtx.target.materialName = m.name;
          targetedMtx.target.indirect = false;
          targetedMtx.target.matrixIndex = i;
          auto sortIndex = calculateSortIndex(m.matrices[j]);
          if (sortIndex < 0) {
            sortIndex = lastSort + 1;
            ++sortBiasCtr;
          } else {
            sortIndex += sortBiasCtr;
          }
          lastSort = sortIndex;
          targetedMtx.matrix = TRY(readMatrix(srt, m.matrices[j++], warn));
          tmp.matrices.push_back(targetedMtx);
          sortIndices.push_back(sortIndex);
        }
      }
      for (size_t i = 0; i < 3; ++i) {
        if (m.enabled_indsrts & (1 << i)) {
          TargetedMtx targetedMtx;
          targetedMtx.target.materialName = m.name;
          targetedMtx.target.indirect = true;
          targetedMtx.target.matrixIndex = i;
          auto sortIndex = calculateSortIndex(m.matrices[j]);
          if (sortIndex < 0) {
            sortIndex = lastSort;
            ++sortBiasCtr;
          } else {
            sortIndex += sortBiasCtr;
          }
          lastSort = sortIndex;
          targetedMtx.matrix = TRY(readMatrix(srt, m.matrices[j++], warn));
          tmp.matrices.push_back(targetedMtx);
          sortIndices.push_back(sortIndex);
        }
      }
    }

    std::vector<size_t> indices(tmp.matrices.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&](size_t i1, size_t i2) {
      return sortIndices[i1] < sortIndices[i2];
    });

    std::vector<TargetedMtx> sortedMatrices;
    for (auto i : indices) {
      sortedMatrices.push_back(tmp.matrices[i]);
    }
    tmp.matrices = sortedMatrices;

    return tmp;
  }
  static BinarySrt write(const SrtAnim& anim) {
    BinarySrt binary;

    // Converting general fields
    binary.name = anim.name;
    binary.sourcePath = anim.sourcePath;
    binary.frameDuration = anim.frameDuration;
    binary.xformModel = anim.xformModel;
    binary.wrapMode = anim.wrapMode;

    std::map<std::string, std::vector<int>> materialSubMatrixSort;

    // Create a map to track material names and their corresponding indices in
    // the binary.materials vector
    std::unordered_map<std::string, int> materialIndexMap;

    // Mapping from SrtAnim::TargetedMtx to SRT0Material, SRT0Matrix, and
    // SRT0Track
    for (const auto& targetedMtx : anim.matrices) {
      if (materialIndexMap.count(targetedMtx.target.materialName) == 0) {
        // Create a new SRT0Material if it doesn't already exist
        SRT0Material material;
        material.name = targetedMtx.target.materialName;
        binary.materials.push_back(material);
        materialIndexMap[material.name] = binary.materials.size() - 1;
      }
      auto& material =
          binary.materials[materialIndexMap[targetedMtx.target.materialName]];
      material.enabled_texsrts |= targetedMtx.target.indirect
                                      ? 0
                                      : (1 << targetedMtx.target.matrixIndex);
      material.enabled_indsrts |= targetedMtx.target.indirect
                                      ? (1 << targetedMtx.target.matrixIndex)
                                      : 0;

      SRT0Matrix matrix;
      matrix.flags = SRT0Matrix::FLAG_ENABLED;

      // Add tracks for scaleX, scaleY, rot, transX, and transY
      std::vector<SRT0Matrix::TargetId> targetIds{
          SRT0Matrix::TargetId::ScaleU, SRT0Matrix::TargetId::ScaleV,
          SRT0Matrix::TargetId::Rotate, SRT0Matrix::TargetId::TransU,
          SRT0Matrix::TargetId::TransV};
      std::vector<const SrtAnim::Track*> tracks{
          &targetedMtx.matrix.scaleX, &targetedMtx.matrix.scaleY,
          &targetedMtx.matrix.rot, &targetedMtx.matrix.transX,
          &targetedMtx.matrix.transY};
      for (size_t i = 0; i < targetIds.size(); ++i) {
        if (auto* f = std::get_if<f32>(tracks[i])) {
          if (targetIds[i] == SRT0Matrix::TargetId::ScaleU) {
            matrix.flags |= SRT0Matrix::FLAG_SCL_U_FIXED;
            if (targetedMtx.matrix.scaleY.index() == 0 &&
                std::get<f32>(targetedMtx.matrix.scaleY) == *f) {
              matrix.flags |= SRT0Matrix::FLAG_SCL_ISOTROPIC;
              if (*f == 1.0f) {
                matrix.flags |= SRT0Matrix::FLAG_SCL_ONE;
              }
            }
          } else if (targetIds[i] == SRT0Matrix::TargetId::ScaleV) {
            matrix.flags |= SRT0Matrix::FLAG_SCL_V_FIXED;
          } else if (targetIds[i] == SRT0Matrix::TargetId::Rotate) {
            matrix.flags |= SRT0Matrix::FLAG_ROT_FIXED;
            if (*f == 0.0f) {
              matrix.flags |= SRT0Matrix::FLAG_ROT_ZERO;
            }
          } else if (targetIds[i] == SRT0Matrix::TargetId::TransU) {
            matrix.flags |= SRT0Matrix::FLAG_TRANS_U_FIXED;
            if (targetedMtx.matrix.transY.index() == 0 &&
                std::get<f32>(targetedMtx.matrix.transY) == *f) {
              if (*f == 0.0f) {
                matrix.flags |= SRT0Matrix::FLAG_TRANS_ZERO;
              }
            }
          } else if (targetIds[i] == SRT0Matrix::TargetId::TransV) {
            matrix.flags |= SRT0Matrix::FLAG_TRANS_V_FIXED;
          }
          if (!matrix.isAttribIncluded(targetIds[i], matrix.flags)) {
            continue;
          }
          SRT0Target target;
          target.data = std::get<f32>(*tracks[i]);
          matrix.targets.push_back(target);
        } else if (std::holds_alternative<std::vector<SRT0KeyFrame>>(
                       *tracks[i])) {
          SRT0Track track;
          track.keyframes = std::get<std::vector<SRT0KeyFrame>>(*tracks[i]);
          track.step =
              CalcStep(track.keyframes.front().frame,
                       track.keyframes.back()
                           .frame); // Make sure CalcStep is implemented
          binary.tracks.push_back(track);

          SRT0Target target;
          target.data = static_cast<u32>(binary.tracks.size() - 1);
          matrix.targets.push_back(target);
        }
      }

      // Add the SRT0Matrix to the corresponding SRT0Material
      material.matrices.push_back(matrix);
      materialSubMatrixSort[material.name].push_back(
          targetedMtx.target.matrixIndex +
          (targetedMtx.target.indirect ? 100 : 0));
    }

    for (auto& mat : binary.materials) {
      auto& sortIndices = materialSubMatrixSort[mat.name];

      std::vector<size_t> indices(sortIndices.size());
      std::iota(indices.begin(), indices.end(), 0);
      std::sort(indices.begin(), indices.end(), [&](size_t i1, size_t i2) {
        return sortIndices[i1] < sortIndices[i2];
      });

      std::vector<SRT0Matrix> sortedMatrices;
      for (auto i : indices) {
        sortedMatrices.push_back(mat.matrices[i]);
      }
      mat.matrices = sortedMatrices;
    }

    binary.mergeIdenticalTracks();

    return binary;
  }

private:
  static int calculateSortIndex(const SRT0Matrix& mtx) {
    int maxIndex = -1;
    for (auto& target : mtx.targets) {
      if (std::holds_alternative<u32>(target.data)) {
        maxIndex =
            std::max(maxIndex, static_cast<int>(std::get<u32>(target.data)));
      }
    }
    return maxIndex;
  }

  static Result<Mtx> readMatrix(const BinarySrt& srt, const SRT0Matrix& mtx,
                                std::function<void(std::string_view)> warn) {
    size_t k = 0;
    Mtx y{};
    if (!mtx.isAttribIncluded(SRT0Matrix::TargetId::ScaleU, mtx.flags)) {
      y.scaleX = 1.0f;
    } else {
      y.scaleX = TRY(readTrack(srt.tracks, mtx.targets[k++], warn));
    }
    if (!mtx.isAttribIncluded(SRT0Matrix::TargetId::ScaleV, mtx.flags)) {
      y.scaleY = 1.0f;
    } else {
      y.scaleY = TRY(readTrack(srt.tracks, mtx.targets[k++], warn));
    }
    if (!mtx.isAttribIncluded(SRT0Matrix::TargetId::Rotate, mtx.flags)) {
      y.rot = 0.0f;
    } else {
      y.rot = TRY(readTrack(srt.tracks, mtx.targets[k++], warn));
    }
    if (!mtx.isAttribIncluded(SRT0Matrix::TargetId::TransU, mtx.flags)) {
      y.transX = 0.0f;
    } else {
      y.transX = TRY(readTrack(srt.tracks, mtx.targets[k++], warn));
    }
    if (!mtx.isAttribIncluded(SRT0Matrix::TargetId::TransV, mtx.flags)) {
      y.transY = 0.0f;
    } else {
      y.transY = TRY(readTrack(srt.tracks, mtx.targets[k++], warn));
    }
    return y;
  }
  static Result<Track> readTrack(std::span<const SRT0Track> tracks,
                                 const SRT0Target& target,
                                 std::function<void(std::string_view)> warn) {
    if (auto* fixed = std::get_if<f32>(&target.data)) {
      return Track{TRY(checkFloat(*fixed))};
    } else {
      assert(std::get_if<u32>(&target.data));
      auto& track = tracks[*std::get_if<u32>(&target.data)];
      EXPECT(track.reserved[0] == 0 && track.reserved[1] == 0);
      EXPECT(tracks.size() >= 1);
      for (auto& f : track.keyframes) {
        TRY(checkFloat(f.frame));
        TRY(checkFloat(f.value));
        TRY(checkFloat(f.tangent));
      }
      auto strictly_increasing =
          std::ranges::adjacent_find(track.keyframes, [](auto& x, auto& y) {
            return x.frame >= y.frame;
          }) == track.keyframes.end();
      EXPECT(strictly_increasing,
             "SRT0 track keyframes must be strictly increasing");
      f32 begin = track.keyframes[0].frame;
      f32 end = track.keyframes.back().frame;
      auto step = CalcStep(begin, end);
      if (track.step != step) {
        warn("Frame interval not properly computed");
      }
      return Track{track.keyframes};
    }
  }

  static Result<f32> checkFloat(f32 in) {
    if (std::isinf(in)) {
      return in > 0.0f ? std::unexpected("Float is set to INFINITY")
                       : std::unexpected("Float is set to -INFINITY");
    }
    if (std::isnan(in)) {
      return in > 0.0f ? std::unexpected("Float is set to NAN")
                       : std::unexpected("Float is set to -NAN");
    }
    return in;
  }
};

using SrtAnimationArchive = SrtAnim;

} // namespace librii::g3d
