#include "AnimIO.hpp"

#include <rsl/ArrayUtil.hpp>
#include <rsl/CheckFloat.hpp>
#include <rsl/CompactVector.hpp>
#include <rsl/SortArray.hpp>

namespace librii::g3d {

struct SRTOffsets {
  s32 ofsBrres{};
  s32 ofsMatDict{};
  s32 ofsUserData{};

  static constexpr size_t size_bytes() { return 3 * 4; }

  Result<void> read(oishii::BinaryReader& reader) {
    rsl::SafeReader safe(reader);
    ofsBrres = TRY(safe.S32());
    ofsMatDict = TRY(safe.S32());
    ofsUserData = TRY(safe.S32());
    return {};
  }
  void write(oishii::Writer& writer) {
    writer.write<s32>(ofsBrres);
    writer.write<s32>(ofsMatDict);
    writer.write<s32>(ofsUserData);
  }
};

struct BinarySrtInfo {
  std::string name;
  std::string sourcePath;
  u16 frameDuration{};
  u16 materialCount{};
  u32 xformModel{};
  AnimationWrapMode wrapMode{AnimationWrapMode::Repeat};

  Result<void> read(oishii::BinaryReader& reader, u32 pat0) {
    rsl::SafeReader safe(reader);
    name = TRY(safe.StringOfs32(pat0));
    sourcePath = TRY(safe.StringOfs32(pat0));
    frameDuration = TRY(safe.U16());
    materialCount = TRY(safe.U16());
    xformModel = TRY(safe.U32());
    wrapMode = TRY(safe.Enum32<AnimationWrapMode>());
    return {};
  }
  void write(oishii::Writer& writer, NameTable& names, u32 pat0) const {
    writeNameForward(names, writer, pat0, name, true);
    writeNameForward(names, writer, pat0, sourcePath, true);
    writer.write<u16>(frameDuration);
    writer.write<u16>(materialCount);
    writer.write<u32>(xformModel);
    writer.write<u32>(static_cast<u32>(wrapMode));
  }
};

Result<void> BinarySrt::read(oishii::BinaryReader& reader) {
  rsl::SafeReader safe(reader);
  auto srt0 = safe.scoped("SRT0");
  TRY(safe.Magic("SRT0"));
  TRY(safe.U32()); // size
  auto ver = TRY(safe.U32());
  if (ver != 5) {
    return std::unexpected(std::format(
        "Unsupported SRT0 version {}. Only version 5 is supported.", ver));
  }
  SRTOffsets offsets;
  TRY(offsets.read(reader));

  BinarySrtInfo info;
  TRY(info.read(reader, srt0.start));
  name = info.name;
  sourcePath = info.sourcePath;
  frameDuration = info.frameDuration;
  wrapMode = info.wrapMode;

  std::vector<u32> debugOfsToTrack;

  auto track_addr_to_index = [&](u32 addr) -> Result<u32> {
    auto back = safe.tell();
    safe.seekSet(addr);
    SRT0Track track;
    TRY(track.read(safe));
    safe.seekSet(back);
    auto it = std::find(tracks.begin(), tracks.end(), track);
    if (it != tracks.end()) {
      return it - tracks.begin();
    }
    tracks.push_back(track);
    debugOfsToTrack.push_back(addr);
    return tracks.size() - 1;
  };

  safe.seekSet(srt0.start + offsets.ofsMatDict);
  auto matDict = TRY(ReadDictionary(safe));
  EXPECT(matDict.nodes.size() == info.materialCount);

  for (const auto& node : matDict.nodes) {
    auto& mat = materials.emplace_back();
    safe.seekSet(node.stream_pos);
    TRY(mat.read(safe, track_addr_to_index));
  }

  // Reorder tracks based on initial ordering.
  // TODO: Figure out the initial sorting algorithm here.
  std::set<u32> sorted(debugOfsToTrack.begin(), debugOfsToTrack.end());
  std::vector<u32> fromToOperation(debugOfsToTrack.size());
  for (size_t i = 0; i < tracks.size(); ++i) {
    auto it = sorted.find(debugOfsToTrack[i]);
    EXPECT(it != sorted.end());
    fromToOperation[i] = std::distance(sorted.begin(), it);
  }

  std::vector<SRT0Track> tmp;
  for (auto index : fromToOperation) {
    tmp.push_back(tracks[index]);
  }
  tracks = tmp;

  for (auto& mat : materials) {
    for (auto& x : mat.matrices) {
      for (auto& t : x.targets) {
        if (auto* u = std::get_if<u32>(&t.data)) {
          *u = fromToOperation[*u];
        }
      }
    }
  }

  return {};
}

Result<void> BinarySrt::write(oishii::Writer& writer, NameTable& names,
                              u32 addrBrres) const {
  auto start = writer.tell();
  writer.write<u32>('SRT0');
  writer.write<u32>(0, false);
  writer.write<u32>(5);
  auto wb = writer.tell();
  SRTOffsets offsets;
  offsets.ofsBrres = addrBrres - start;
  writer.skip(offsets.size_bytes());

  BinarySrtInfo info{
      .name = name,
      .sourcePath = sourcePath,
      .frameDuration = frameDuration,
      .materialCount = static_cast<u16>(materials.size()),
      .xformModel = xformModel,
      .wrapMode = wrapMode,
  };
  info.write(writer, names, start);

  BetterDictionary dict;

  std::vector<u32> track_addresses;
  // Edge case: no root node if 1 entry
  auto dictSize = CalcDictionarySize(materials.size());
  u32 accum = start + 0x2C /* header */ + dictSize;
  for (auto& mat : materials) {
    dict.nodes.push_back(BetterNode{.name = mat.name, .stream_pos = accum});
    accum += mat.computeSize();
  }
  for (auto& track : tracks) {
    track_addresses.push_back(accum);
    accum += track.computeSize();
  }
  auto track_index_to_addr = [&](u32 i) { return track_addresses[i]; };

  offsets.ofsMatDict = writer.tell() - start;
  WriteDictionary(dict, writer, names);
  for (auto& mat : materials) {
    TRY(mat.write(writer, names, track_index_to_addr));
  }
  for (auto& track : tracks) {
    track.write(writer);
  }

  auto back = writer.tell();
  writer.seekSet(wb);
  offsets.write(writer);
  writer.seekSet(start + 4);
  writer.write<u32>(back - start);
  writer.seekSet(back);

  return {};
}

u32 SRT0Track::computeSize() const { return 8 + keyframes.size() * 12; }
Result<void> SRT0Track::read(rsl::SafeReader& safe) {
  auto numFrames = TRY(safe.U16());
  for (auto& e : reserved) {
    e = TRY(safe.U8());
  }
  step = TRY(safe.F32());
  for (u32 i = 0; i < numFrames; ++i) {
    f32 frame = TRY(safe.F32());
    f32 value = TRY(safe.F32());
    f32 tangent = TRY(safe.F32());
    keyframes.push_back(SRT0KeyFrame{
        .frame = frame,
        .value = value,
        .tangent = tangent,
    });
  }
  return {};
}
void SRT0Track::write(oishii::Writer& writer) const {
  writer.write<u16>(keyframes.size());
  for (auto e : reserved) {
    writer.write<u8>(e);
  }
  writer.write<f32>(step);
  for (const auto& data : keyframes) {
    writer.write<f32>(data.frame);
    writer.write<f32>(data.value);
    writer.write<f32>(data.tangent);
  }
}
u32 SRT0Matrix::computeSize() const {
  u32 accum = 4;
  for (u32 i = 0; i < targets.size(); ++i) {
    accum += 4;
  }
  return accum;
}

Result<void>
SRT0Matrix::read(rsl::SafeReader& safe,
                 std::function<Result<u32>(u32)> trackAddressToIndex) {
  auto matrix = safe.scoped("SRT0Matrix");
  flags = TRY(safe.U32());
  for (u32 i = 0; i < static_cast<u32>(TargetId::Count); ++i) {
    if (0 == (flags & (FLAG_ENABLED << (i * 0)))) {
      continue;
    }
    auto attrib = static_cast<TargetId>(i);
    bool included = isAttribIncluded(attrib, flags);
    bool fixed = isFixed(attrib, flags);
    if (!included) {
      continue;
    }
    if (fixed) {
      f32 constFrame = TRY(safe.F32());
      targets.emplace_back(SRT0Target{.data = constFrame});
    } else {
      auto base = safe.tell();
      auto ofs = TRY(safe.S32());
      // For some reason it's relative to the entry entry in SRT0. In
      // PAT0 it's relative to section start.
      u32 addr = base + ofs;
      u32 index = TRY(trackAddressToIndex(addr));
      targets.emplace_back(SRT0Target{.data = index});
    }
  }
  return {};
}
void SRT0Matrix::write(oishii::Writer& writer,
                       std::function<u32(u32)> trackIndexToAddress) const {
  writer.write<u32>(flags);
  for (auto& target : targets) {
    if (auto* c = std::get_if<f32>(&target.data)) {
      writer.write<f32>(*c);
    } else if (auto* index = std::get_if<u32>(&target.data)) {
      u32 addr = trackIndexToAddress(*index);
      // Relative to offset itself. See comment above.
      auto ofs = addr - writer.tell();
      writer.write<s32>(ofs);
    } else {
      assert(!"Internal error");
    }
  }
}
u32 SRT0Material::computeSize() const {
  return std::accumulate(matrices.begin(), matrices.end(), static_cast<u32>(12),
                         [&](u32 count, auto& x) -> u32 {
                           return count + x.computeSize() +
                                  4 /* For the offset */;
                         });
}
Result<void>
SRT0Material::read(rsl::SafeReader& reader,
                   std::function<Result<u32>(u32)> trackAddressToIndex) {
  auto start = reader.tell();
  name = TRY(reader.StringOfs(start));
  enabled_texsrts = TRY(reader.U32());
  enabled_indsrts = TRY(reader.U32());
  for (u32 i = 0; i < 8; ++i) {
    if (0 == (enabled_texsrts & (FLAG_ENABLED << (i * 1)))) {
      continue;
    }
    {
      auto ofs = TRY(reader.S32());
      u32 at = start + ofs;
      auto back = reader.tell();
      reader.seekSet(at);
      SRT0Matrix mtx{};
      TRY(mtx.read(reader, trackAddressToIndex));
      matrices.push_back(mtx);
      reader.seekSet(back);
    }
  }
  for (u32 i = 0; i < 3; ++i) {
    if (0 == (enabled_indsrts & (FLAG_ENABLED << (i * 1)))) {
      continue;
    }
    {
      auto ofs = TRY(reader.S32());
      u32 at = start + ofs;
      auto back = reader.tell();
      reader.seekSet(at);
      SRT0Matrix mtx{};
      TRY(mtx.read(reader, trackAddressToIndex));
      matrices.push_back(mtx);
      reader.seekSet(back);
    }
  }
  return {};
}
Result<void>
SRT0Material::write(oishii::Writer& writer, NameTable& names,
                    std::function<u32(u32)> trackIndexToAddress) const {
  auto start = writer.tell();
  writeNameForward(names, writer, start, name, true);
  writer.write<u32>(enabled_texsrts);
  writer.write<u32>(enabled_indsrts);

  u32 count = std::popcount(enabled_texsrts) + std::popcount(enabled_indsrts);
  EXPECT(count == matrices.size());
  u32 accum = start + 12 + count * 4;
  std::vector<u32> matrixAddrs;
  for (u32 i = 0; i < count; ++i) {
    matrixAddrs.push_back(accum);
    accum += matrices[i].computeSize();
  }

  for (u32 i = 0; i < count; ++i) {
    u32 addr = matrixAddrs[i];
    auto ofs = addr - start;
    writer.write<s32>(ofs);
  }
  for (auto& mtx : matrices) {
    mtx.write(writer, trackIndexToAddress);
  }
  EXPECT(writer.tell() == accum);
  return {};
}

void BinarySrt::mergeIdenticalTracks() {
  auto compacted = rsl::StableCompactVector(tracks);

  // Replace old track indices with new indices in SRT0Matrix targets
  for (auto& material : materials) {
    for (auto& matrix : material.matrices) {
      for (auto& target : matrix.targets) {
        if (auto* index = std::get_if<u32>(&target.data)) {
          *index = compacted.remapTableOldToNew[*index];
        }
      }
    }
  }

  // Replace the old tracks with the new list of unique tracks
  tracks = std::move(compacted.uniqueElements);
}

///////////////////////
// JSON <=> Binary
///////////////////////

// Implementation details for reading/writing a SrtAnim
class ConverterSrtAnim : public SrtAnim {
public:
  ConverterSrtAnim() = default;

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
    // Calling rsl::RemoveSortPlaceholdersInplace will produce
    // 0  1  2  3  4  5  6  7  8
    //
    std::vector<int> sortIndices;

    for (const auto& m : srt.materials) {
      size_t j = 0;
      for (size_t i = 0; i < 8; ++i) {
        if (m.enabled_texsrts & (1 << i)) {
          TargetedMtx targetedMtx;
          targetedMtx.target.materialName = m.name;
          targetedMtx.target.indirect = false;
          targetedMtx.target.matrixIndex = i;
          // NB: This index may be negative (placeholder)
          auto sortIndex = calculateSortIndex(m.matrices[j]);
          sortIndices.push_back(sortIndex);
          targetedMtx.matrix = TRY(readMatrix(srt, m.matrices[j], warn));
          tmp.matrices.push_back(targetedMtx);
          ++j;
        }
      }
      for (size_t i = 0; i < 3; ++i) {
        if (m.enabled_indsrts & (1 << i)) {
          TargetedMtx targetedMtx;
          targetedMtx.target.materialName = m.name;
          targetedMtx.target.indirect = true;
          targetedMtx.target.matrixIndex = i;
          // NB: This index may be negative (placeholder)
          auto sortIndex = calculateSortIndex(m.matrices[j]);
          sortIndices.push_back(sortIndex);
          targetedMtx.matrix = TRY(readMatrix(srt, m.matrices[j], warn));
          tmp.matrices.push_back(targetedMtx);
          ++j;
        }
      }
    }

    rsl::RemoveSortPlaceholdersInplace(sortIndices);
    tmp.matrices = rsl::SortArrayByKeys(tmp.matrices, sortIndices);

    return tmp;
  }
  // When an attribute is *not* animated, we need to grab a value from the
  // model..
  static std::optional<float> getDefaultAttrib(auto* mdl, auto& material,
                                               auto& targetedMtx, int i) {
    if (mdl == nullptr) {
      return std::nullopt;
    }
    auto* m = findByName2(mdl->materials, material.name);
    if (m == nullptr) {
      return std::nullopt;
    }
    auto midx = targetedMtx.target.matrixIndex;
    if (targetedMtx.target.indirect) {
      if (midx <= m->mIndMatrices.size()) {
        switch (i) {
        case 0:
          return m->mIndMatrices[midx].scale.x;
        case 1:
          return m->mIndMatrices[midx].scale.y;
        case 2:
          return m->mIndMatrices[midx].rotate;
        case 3:
          return m->mIndMatrices[midx].trans.x;
        case 4:
          return m->mIndMatrices[midx].trans.y;
        }
      }
    } else {
      if (midx <= m->texMatrices.size()) {
        switch (i) {
        case 0:
          return m->texMatrices[midx].scale.x;
        case 1:
          return m->texMatrices[midx].scale.y;
        case 2:
          return m->texMatrices[midx].rotate;
        case 3:
          return m->texMatrices[midx].translate.x;
        case 4:
          return m->texMatrices[midx].translate.y;
        }
      }
    }
    return std::nullopt;
  }
  static BinarySrt write(const SrtAnim& anim,
                         const librii::g3d::Model* mdl = nullptr) {
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
        SrtAnim::Track track = *tracks[i];
        bool isTrackAnimated = track.size() > 1;
        if (!isTrackAnimated) {
          f32 f{};

          bool isConstantValue = track.size() == 1;
          if (isConstantValue) {
            f = track[0].value;
          } else if (auto model_value =
                         getDefaultAttrib(mdl, material, targetedMtx, i);
                     model_value) {
            // Grab value from model
            f = *model_value;
          } else {
            // Backup values if cannot retrieve from model
            // - Scale:     1.0f
            // - Rot/Trans: 0.0f
            bool isScaleTargetId = i <= 2;
            f = isScaleTargetId ? 1.0f : 0.0f;
          }

          if (targetIds[i] == SRT0Matrix::TargetId::ScaleU) {
            matrix.flags |= SRT0Matrix::FLAG_SCL_U_FIXED;
            if (targetedMtx.matrix.scaleY.size() == 1 &&
                targetedMtx.matrix.scaleY[0].value == f) {
              matrix.flags |= SRT0Matrix::FLAG_SCL_ISOTROPIC;
              if (f == 1.0f) {
                matrix.flags |= SRT0Matrix::FLAG_SCL_ONE;
              }
            }
          } else if (targetIds[i] == SRT0Matrix::TargetId::ScaleV) {
            matrix.flags |= SRT0Matrix::FLAG_SCL_V_FIXED;
          } else if (targetIds[i] == SRT0Matrix::TargetId::Rotate) {
            matrix.flags |= SRT0Matrix::FLAG_ROT_FIXED;
            if (f == 0.0f) {
              matrix.flags |= SRT0Matrix::FLAG_ROT_ZERO;
            }
          } else if (targetIds[i] == SRT0Matrix::TargetId::TransU) {
            matrix.flags |= SRT0Matrix::FLAG_TRANS_U_FIXED;
            if (targetedMtx.matrix.transY.size() == 1 &&
                targetedMtx.matrix.transY[0].value == f) {
              if (f == 0.0f) {
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
          target.data = f;
          matrix.targets.push_back(target);
        } else {
          SRT0Track track;
          track.keyframes = *tracks[i];
          track.step = CalcStep(track.keyframes.front().frame,
                                track.keyframes.back().frame);
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
      // Note: matrixIndex in [0, 7]. 100 is arbitrary and 7 would've worked too
    }

    for (auto& mat : binary.materials) {
      const auto& sortIndices = materialSubMatrixSort[mat.name];

      mat.matrices = rsl::SortArrayByKeys(mat.matrices, sortIndices);
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
      y.scaleX = {SRT0KeyFrame{.value = 1.0f}};
    } else {
      y.scaleX = TRY(readTrack(srt.tracks, mtx.targets[k++], warn));
    }
    if (!mtx.isAttribIncluded(SRT0Matrix::TargetId::ScaleV, mtx.flags)) {
      auto scl =
          (mtx.flags & SRT0Matrix::FLAG_SCL_ISOTROPIC) ? y.scaleX[0].value : 0;
      y.scaleY = {SRT0KeyFrame{.value = scl}};
    } else {
      y.scaleY = TRY(readTrack(srt.tracks, mtx.targets[k++], warn));
    }
    if (!mtx.isAttribIncluded(SRT0Matrix::TargetId::Rotate, mtx.flags)) {
      y.rot = {SRT0KeyFrame{.value = 0.0f}};
    } else {
      y.rot = TRY(readTrack(srt.tracks, mtx.targets[k++], warn));
    }
    if (!mtx.isAttribIncluded(SRT0Matrix::TargetId::TransU, mtx.flags)) {
      y.transX = {SRT0KeyFrame{.value = 0.0f}};
    } else {
      y.transX = TRY(readTrack(srt.tracks, mtx.targets[k++], warn));
    }
    if (!mtx.isAttribIncluded(SRT0Matrix::TargetId::TransV, mtx.flags)) {
      y.transY = {SRT0KeyFrame{.value = 0.0f}};
    } else {
      y.transY = TRY(readTrack(srt.tracks, mtx.targets[k++], warn));
    }
    return y;
  }
  static Result<Track> readTrack(std::span<const SRT0Track> tracks,
                                 const SRT0Target& target,
                                 std::function<void(std::string_view)> warn) {
    if (auto* fixed = std::get_if<f32>(&target.data)) {
      return Track{SRT0KeyFrame{.value = TRY(rsl::CheckFloat(*fixed))}};
    } else {
      assert(std::get_if<u32>(&target.data));
      auto& track = tracks[*std::get_if<u32>(&target.data)];
      EXPECT(track.reserved[0] == 0 && track.reserved[1] == 0);
      EXPECT(tracks.size() >= 1);
      for (auto& f : track.keyframes) {
        TRY(rsl::CheckFloat(f.frame));
        TRY(rsl::CheckFloat(f.value));
        TRY(rsl::CheckFloat(f.tangent));
      }
      auto monotonic_increasing =
          std::ranges::adjacent_find(track.keyframes, [](auto& x, auto& y) {
            return x.frame > y.frame;
          }) == track.keyframes.end();
      EXPECT(monotonic_increasing, "SRT0 track keyframes must be increasing");
      f32 begin = track.keyframes[0].frame;
      f32 end = track.keyframes.back().frame;
      auto step = CalcStep(begin, end);
      if (track.step != step) {
        warn("Frame interval not properly computed");
      }
      EXPECT(track.keyframes.size() > 1,
             "Unexpected animation track with a single keyframe, but not "
             "encoded as a FIXED value.");
      return Track{track.keyframes};
    }
  }
};

Result<SrtAnim> SrtAnim::read(const BinarySrt& srt,
                              std::function<void(std::string_view)> warn) {
  ConverterSrtAnim converter;
  return converter.read(srt, warn);
}
BinarySrt SrtAnim::write(const SrtAnim& anim, const librii::g3d::Model* mdl) {
  ConverterSrtAnim converter;
  return converter.write(anim, mdl);
}

} // namespace librii::g3d
