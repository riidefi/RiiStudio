#include "AnimTexPatIO.hpp"

#include <rsl/CompactVector.hpp>

namespace librii::g3d {

struct BinaryTexPatInfo {
  std::string name;
  std::string sourcePath;
  u16 frameDuration{};
  u16 materialCount{};
  u16 textureCount{};
  u16 paletteCount{};
  AnimationWrapMode wrapMode{AnimationWrapMode::Repeat};

  Result<void> read(rsl::SafeReader& reader, u32 pat0) {
    name = TRY(reader.StringOfs(pat0));
    sourcePath = TRY(reader.StringOfs(pat0));
    frameDuration = TRY(reader.U16());
    materialCount = TRY(reader.U16());
    textureCount = TRY(reader.U16());
    paletteCount = TRY(reader.U16());
    wrapMode = TRY(reader.Enum32<AnimationWrapMode>());
    return {};
  }
  void write(oishii::Writer& writer, NameTable& names, u32 pat0) const {
    writeNameForward(names, writer, pat0, name, true);
    writeNameForward(names, writer, pat0, sourcePath, true);
    writer.write<u16>(frameDuration);
    writer.write<u16>(materialCount);
    writer.write<u16>(textureCount);
    writer.write<u16>(paletteCount);
    writer.write<u32>(static_cast<u32>(wrapMode));
  }
};

struct PATOffsets {
  s32 ofsBrres{};
  s32 ofsMatDict{};
  s32 ofsTexNames{};
  s32 ofsPlltNames{};
  s32 ofsRuntimeTex{};
  s32 ofsRuntimePltt{};
  s32 ofsUserData{};

  static constexpr size_t size_bytes() { return 7 * 4; }

  Result<void> read(rsl::SafeReader& reader) {
    ofsBrres = TRY(reader.S32());
    ofsMatDict = TRY(reader.S32());
    ofsTexNames = TRY(reader.S32());
    ofsPlltNames = TRY(reader.S32());
    ofsRuntimeTex = TRY(reader.S32());
    ofsRuntimePltt = TRY(reader.S32());
    ofsUserData = TRY(reader.S32());
    return {};
  }
  void write(oishii::Writer& writer) {
    writer.write<s32>(ofsBrres);
    writer.write<s32>(ofsMatDict);
    writer.write<s32>(ofsTexNames);
    writer.write<s32>(ofsPlltNames);
    writer.write<s32>(ofsRuntimeTex);
    writer.write<s32>(ofsRuntimePltt);
    writer.write<s32>(ofsUserData);
  }
};

Result<void> BinaryTexPat::read(oishii::BinaryReader& unsafeReader) {
  rsl::SafeReader reader(unsafeReader);
  auto start = reader.tell();
  TRY(reader.Magic("PAT0"));
  TRY(reader.U32()); // size
  auto ver = TRY(reader.U32());
  if (ver != 4) {
    return std::unexpected(std::format(
        "Unsupported PAT0 version {}. Only version 4 is supported", ver));
  }
  PATOffsets offsets;
  offsets.read(reader);

  BinaryTexPatInfo info;
  info.read(reader, start);
  name = info.name;
  sourcePath = info.sourcePath;
  frameDuration = info.frameDuration;
  wrapMode = info.wrapMode;

  auto track_addr_to_index = [&](u32 addr) -> Result<u32> {
    auto back = reader.tell();
    reader.seekSet(addr);
    PAT0Track track;
    TRY(track.read(reader));
    reader.seekSet(back);
    auto it = std::find(tracks.begin(), tracks.end(), track);
    if (it != tracks.end()) {
      return it - tracks.begin();
    }
    tracks.push_back(track);
    return tracks.size() - 1;
  };

  reader.seekSet(start + offsets.ofsMatDict);
  auto matDict = TRY(ReadDictionary(reader));
  EXPECT(matDict.nodes.size() == info.materialCount);

  for (const auto& node : matDict.nodes) {
    auto& mat = materials.emplace_back();
    reader.seekSet(node.stream_pos);
    TRY(mat.read(reader, track_addr_to_index));
  }

  // Assume numNames == numRuntimePtrs
  reader.seekSet(start + offsets.ofsTexNames);
  for (u16 i = 0; i < info.textureCount; ++i) {
    textureNames.emplace_back(
        TRY(reader.StringOfs(start + offsets.ofsTexNames)));
  }
  reader.seekSet(start + offsets.ofsPlltNames);
  for (u16 i = 0; i < info.paletteCount; ++i) {
    paletteNames.emplace_back(
        TRY(reader.StringOfs(start + offsets.ofsPlltNames)));
  }
  reader.seekSet(start + offsets.ofsRuntimeTex);
  for (u16 i = 0; i < info.textureCount; ++i) {
    runtimeTextures.push_back(TRY(reader.U32()));
  }
  reader.seekSet(start + offsets.ofsRuntimePltt);
  for (u16 i = 0; i < info.paletteCount; ++i) {
    runtimePalettes.push_back(TRY(reader.U32()));
  }
  return {};
}

Result<void> BinaryTexPat::write(oishii::Writer& writer, NameTable& names,
                                 u32 addrBrres) const {
  auto start = writer.tell();
  writer.write<u32>('PAT0');
  writer.write<u32>(0, false);
  writer.write<u32>(4);
  auto wb = writer.tell();
  PATOffsets offsets;
  offsets.ofsBrres = addrBrres - start;
  writer.skip(offsets.size_bytes());

  EXPECT(runtimeTextures.size() == textureNames.size());
  EXPECT(runtimePalettes.size() == paletteNames.size());

  BinaryTexPatInfo info{
      .name = name,
      .sourcePath = sourcePath,
      .frameDuration = frameDuration,
      .materialCount = static_cast<u16>(materials.size()),
      .textureCount = static_cast<u16>(runtimeTextures.size()),
      .paletteCount = static_cast<u16>(runtimePalettes.size()),
      .wrapMode = wrapMode,
  };
  info.write(writer, names, start);

  BetterDictionary dict;

  std::vector<u32> track_addresses;
  u32 accum = start + 0x3c /* header */ + CalcDictionarySize(materials.size());
  for (auto& mat : materials) {
    dict.nodes.push_back(BetterNode{.name = mat.name, .stream_pos = accum});
    accum += 8 + 4 * mat.samplers.size();
  }
  for (auto& track : tracks) {
    track_addresses.push_back(accum);
    accum += 8 + 8 * track.keyframes.size();
  }
  auto track_index_to_addr = [&](u32 i) { return track_addresses[i]; };

  offsets.ofsMatDict = writer.tell() - start;
  WriteDictionary(dict, writer, names);
  for (auto& mat : materials) {
    mat.write(writer, names, track_index_to_addr);
  }
  for (auto& track : tracks) {
    track.write(writer);
  }

  offsets.ofsTexNames = writer.tell() - start;
  auto texNameStart = writer.tell();
  for (auto& name : textureNames) {
    writeNameForward(names, writer, texNameStart, name, false);
  }
  offsets.ofsPlltNames = writer.tell() - start;
  auto paletteNameStart = writer.tell();
  for (auto& name : paletteNames) {
    writeNameForward(names, writer, paletteNameStart, name, false);
  }
  offsets.ofsRuntimeTex = writer.tell() - start;
  for (auto& ptr : runtimeTextures) {
    writer.write<u32>(ptr);
  }
  offsets.ofsRuntimePltt = writer.tell() - start;
  for (auto& ptr : runtimePalettes) {
    writer.write<u32>(ptr);
  }
  auto back = writer.tell();
  writer.seekSet(wb);
  offsets.write(writer);
  writer.seekSet(start + 4);
  writer.write<u32>(back - start);
  writer.seekSet(back);

  return {};
}

void BinaryTexPat::mergeIdenticalTracks() {
  auto compacted = rsl::StableCompactVector(tracks);

  // Replace old track indices with new indices in PAT0Material targets
  for (auto& material : materials) {
    for (auto& target : material.samplers) {
      if (auto* index = std::get_if<u32>(&target)) {
        *index = compacted.remapTableOldToNew[*index];
      }
    }
  }

  // Replace the old tracks with the new list of unique tracks
  tracks = std::move(compacted.uniqueElements);
}

} // namespace librii::g3d
