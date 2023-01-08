#include "AnimClrIO.hpp"

namespace librii::g3d {

struct CLROffsets {
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

struct BinaryClrInfo {
  std::string name;
  std::string sourcePath;
  u16 frameDuration{};
  u16 materialCount{};
  AnimationWrapMode wrapMode{AnimationWrapMode::Repeat};

  Result<void> read(oishii::BinaryReader& reader, u32 pat0) {
    rsl::SafeReader safe(reader);
    name = TRY(safe.StringOfs32(pat0));
    sourcePath = TRY(safe.StringOfs32(pat0));
    frameDuration = TRY(safe.U16());
    materialCount = TRY(safe.U16());
    wrapMode = TRY(safe.Enum32<AnimationWrapMode>());
    return {};
  }
  void write(oishii::Writer& writer, NameTable& names, u32 pat0) const {
    writeNameForward(names, writer, pat0, name, true);
    writeNameForward(names, writer, pat0, sourcePath, true);
    writer.write<u16>(frameDuration);
    writer.write<u16>(materialCount);
    writer.write<u32>(static_cast<u32>(wrapMode));
  }
};

Result<void> BinaryClr::read(oishii::BinaryReader& reader) {
  rsl::SafeReader safe(reader);
  auto clr0 = safe.scoped("CLR0");
  TRY(safe.Magic("CLR0"));
  TRY(safe.U32()); // size
  auto ver = TRY(safe.U32());
  if (ver != 4) {
    return std::unexpected(std::format(
        "Unsupported CLR0 version {}. Only version 4 is supported.", ver));
  }
  CLROffsets offsets;
  TRY(offsets.read(reader));

  BinaryClrInfo info;
  TRY(info.read(reader, clr0.start));
  name = info.name;
  sourcePath = info.sourcePath;
  frameDuration = info.frameDuration;
  wrapMode = info.wrapMode;

  auto track_addr_to_index = [&](u32 addr) -> std::expected<u32, std::string> {
    auto back = safe.tell();
    reader.seekSet(addr);
    CLR0Track track;
    // This is inclusive uppper bound because ????
    TRY(track.read(safe, info.frameDuration + 1));
    reader.seekSet(back);
    auto it = std::find(tracks.begin(), tracks.end(), track);
    if (it != tracks.end()) {
      return it - tracks.begin();
    }
    tracks.push_back(track);
    return tracks.size() - 1;
  };

  reader.seekSet(clr0.start + offsets.ofsMatDict);
  auto matDict = TRY(ReadDictionary(safe));
  EXPECT(matDict.nodes.size() == info.materialCount);

  for (const auto& node : matDict.nodes) {
    auto& mat = materials.emplace_back();
    reader.seekSet(node.stream_pos);
    TRY(mat.read(safe, track_addr_to_index));
  }
  return {};
}

void BinaryClr::write(oishii::Writer& writer, NameTable& names,
                      u32 addrBrres) const {
  auto start = writer.tell();
  writer.write<u32>('CLR0');
  writer.write<u32>(0, false);
  writer.write<u32>(4);
  auto wb = writer.tell();
  CLROffsets offsets;
  offsets.ofsBrres = addrBrres - start;
  writer.skip(offsets.size_bytes());

  BinaryClrInfo info{
      .name = name,
      .sourcePath = sourcePath,
      .frameDuration = frameDuration,
      .materialCount = static_cast<u16>(materials.size()),
      .wrapMode = wrapMode,
  };
  info.write(writer, names, start);

  BetterDictionary dict;

  std::vector<u32> track_addresses;
  // Edge case: no root node if 1 entry
  auto dictSize = CalcDictionarySize(materials.size());
  u32 accum = start + 0x28 /* header */ + dictSize;
  for (auto& mat : materials) {
    dict.nodes.push_back(BetterNode{.name = mat.name, .stream_pos = accum});
    accum += 8 + 8 * mat.targets.size();
  }
  for (auto& track : tracks) {
    track_addresses.push_back(accum);
    accum += 4 * track.keyframes.size();
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

  auto back = writer.tell();
  writer.seekSet(wb);
  offsets.write(writer);
  writer.seekSet(start + 4);
  writer.write<u32>(back - start);
  writer.seekSet(back);
}

} // namespace librii::g3d
