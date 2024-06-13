#include "AnimVisIO.hpp"

namespace librii::g3d {

struct VISOffsets {
  s32 ofsBrres{};
  s32 ofsBoneDict{};
  s32 ofsUserData{};

  static constexpr size_t size_bytes() { return 3 * 4; }

  Result<void> read(rsl::SafeReader& reader) {
    ofsBrres = TRY(reader.S32());
    ofsBoneDict = TRY(reader.S32());
    ofsUserData = TRY(reader.S32());
    return {};
  }
  void write(oishii::Writer& writer) {
    writer.write<s32>(ofsBrres);
    writer.write<s32>(ofsBoneDict);
    writer.write<s32>(ofsUserData);
  }
};

struct BinaryVisInfo {
  std::string name;
  std::string sourcePath;
  u16 frameDuration{};
  u16 boneCount{};
  AnimationWrapMode wrapMode{AnimationWrapMode::Repeat};

  Result<void> read(rsl::SafeReader& reader, u32 pat0) {
    name = TRY(reader.StringOfs(pat0));
    sourcePath = TRY(reader.StringOfs(pat0));
    frameDuration = TRY(reader.U16());
    boneCount = TRY(reader.U16());
    wrapMode = TRY(reader.Enum32<AnimationWrapMode>());
    return {};
  }
  void write(oishii::Writer& writer, NameTable& names, u32 pat0) const {
    writeNameForward(names, writer, pat0, name, true);
    writeNameForward(names, writer, pat0, sourcePath, true);
    writer.write<u16>(frameDuration);
    writer.write<u16>(boneCount);
    writer.write<u32>(static_cast<u32>(wrapMode));
  }
};

Result<void> BinaryVis::read(oishii::BinaryReader& unsafeReader) {
  rsl::SafeReader reader(unsafeReader);
  auto start = reader.tell();
  TRY(reader.Magic("VIS0"));
  TRY(reader.U32()); // size
  auto ver = TRY(reader.U32());
  if (ver != 4) {
    return RSL_UNEXPECTED(std::format(
        "Unsupported VIS0 version {}. Only version 4 is supported.", ver));
  }
  VISOffsets offsets;
  TRY(offsets.read(reader));

  BinaryVisInfo info;
  TRY(info.read(reader, start));
  name = info.name;
  sourcePath = info.sourcePath;
  frameDuration = info.frameDuration;
  wrapMode = info.wrapMode;

  reader.seekSet(start + offsets.ofsBoneDict);
  auto boneDict = TRY(ReadDictionary(reader));
  EXPECT(boneDict.nodes.size() == info.boneCount);

  auto realNumKeyFrames = roundUp(info.frameDuration + 1, 32) / 32;

  for (const auto& node : boneDict.nodes) {
    auto& bone = bones.emplace_back();
    reader.seekSet(node.stream_pos);
    TRY(bone.read(reader, realNumKeyFrames));
  }

  return {};
}

void BinaryVis::write(oishii::Writer& writer, NameTable& names,
                      u32 addrBrres) const {
  auto start = writer.tell();
  writer.write<u32>('VIS0');
  writer.write<u32>(0, false);
  writer.write<u32>(4);
  auto wb = writer.tell();
  VISOffsets offsets;
  offsets.ofsBrres = addrBrres - start;
  writer.skip(offsets.size_bytes());

  BinaryVisInfo info{
      .name = name,
      .sourcePath = sourcePath,
      .frameDuration = frameDuration,
      .boneCount = static_cast<u16>(bones.size()),
      .wrapMode = wrapMode,
  };
  info.write(writer, names, start);

  BetterDictionary dict;

  std::vector<u32> track_addresses;
  // Edge case: no root node if 1 entry
  auto dictSize = CalcDictionarySize(bones.size());
  u32 accum = start + 0x28 /* header */ + dictSize;
  for (auto& bone : bones) {
    dict.nodes.push_back(BetterNode{.name = bone.name, .stream_pos = accum});
    accum +=
        8 + (bone.target.has_value() ? bone.target->keyframes.size() * 4 : 0);
  }

  offsets.ofsBoneDict = writer.tell() - start;
  WriteDictionary(dict, writer, names);
  for (auto& bone : bones) {
    bone.write(writer, names);
  }

  auto back = writer.tell();
  writer.seekSet(wb);
  offsets.write(writer);
  writer.seekSet(start + 4);
  writer.write<u32>(back - start);
  writer.seekSet(back);
}

} // namespace librii::g3d
