#include "AnimChrIO.hpp"

namespace librii::g3d {

struct CHR0Offsets {
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

struct BinaryChrInfo {
  std::string name;
  std::string sourcePath;
  u16 frameDuration{};
  u16 materialCount{};
  AnimationWrapMode wrapMode{AnimationWrapMode::Repeat};
  u32 scaleRule = 0;

  Result<void> read(oishii::BinaryReader& reader, u32 pat0) {
    rsl::SafeReader safe(reader);
    name = TRY(safe.StringOfs32(pat0));
    sourcePath = TRY(safe.StringOfs32(pat0));
    frameDuration = TRY(safe.U16());
    materialCount = TRY(safe.U16());
    wrapMode = TRY(safe.Enum32<AnimationWrapMode>());
    scaleRule = TRY(safe.U32());
    return {};
  }
  void write(oishii::Writer& writer, NameTable& names, u32 pat0) const {
    writeNameForward(names, writer, pat0, name, true);
    writeNameForward(names, writer, pat0, sourcePath, true);
    writer.write<u16>(frameDuration);
    writer.write<u16>(materialCount);
    writer.write<u32>(static_cast<u32>(wrapMode));
    writer.write<u32>(scaleRule);
  }
};

Result<void> BinaryChr::read(oishii::BinaryReader& reader) {
  rsl::SafeReader safe(reader);
  auto clr0 = safe.scoped("CHR0");
  TRY(safe.Magic("CHR0"));
  TRY(safe.U32()); // size
  auto ver = TRY(safe.U32());
  if (ver != 5) {
    return std::unexpected(std::format(
        "Unsupported CHR0 version {}. Only version 5 is supported.", ver));
  }
  CHR0Offsets offsets;
  TRY(offsets.read(reader));

  BinaryChrInfo info;
  TRY(info.read(reader, clr0.start));
  name = info.name;
  sourcePath = info.sourcePath;
  frameDuration = info.frameDuration;
  wrapMode = info.wrapMode;
  scaleRule = info.scaleRule;

  reader.seekSet(clr0.start + offsets.ofsMatDict);
  auto matDict = TRY(ReadDictionary(safe));
  EXPECT(matDict.nodes.size() == info.materialCount);

  std::map<u32, CHR0Flags::RotFmt> tracks_;
  for (const auto& node : matDict.nodes) {
    auto& mat = nodes.emplace_back();
    reader.seekSet(node.stream_pos);
    mat = TRY(CHR0Node::read(safe, tracks_));
  }
  // std::map sorted by operator< on key
  for (auto [ofs, fmt] : tracks_) {
    auto& track = tracks.emplace_back();
    reader.seekSet(ofs);
    bool baked = false;
    u32 tp = 0;
    switch (fmt) {
    case CHR0Flags::RotFmt::Const:
      EXPECT(false && "Invalid format specifier");
      break;
    case CHR0Flags::RotFmt::_32:
      baked = false;
      tp = 0;
      break;
    case CHR0Flags::RotFmt::_48:
      baked = false;
      tp = 1;
      break;
    case CHR0Flags::RotFmt::_96:
      baked = false;
      tp = 2;
      break;
    case CHR0Flags::RotFmt::Baked8:
      baked = true;
      tp = 0;
      break;
    case CHR0Flags::RotFmt::Baked16:
      baked = true;
      tp = 1;
      break;
    case CHR0Flags::RotFmt::Baked32:
      baked = true;
      tp = 2;
      break;
    }
    track = TRY(CHR0AnyTrack::read(safe, baked, tp, frameDuration));

    // The slow part
    // TODO: For now, we don't even convert offsets to indices
#if 0
    for (auto& n : nodes) {
      for (auto& t : n.tracks) {
        if (auto* o = std::get_if<u32>(&t)) {
          if (*o == ofs) {
            *o = tracks.size() - 1;
          }
        }
      }
    }
#endif
  }

  return {};
}

void BinaryChr::write(oishii::Writer& writer, NameTable& names,
                      u32 addrBrres) const {
  auto start = writer.tell();
  writer.write<u32>('CHR0');
  writer.write<u32>(0, false);
  writer.write<u32>(5);
  auto wb = writer.tell();
  CHR0Offsets offsets;
  offsets.ofsBrres = addrBrres - start;
  writer.skip(offsets.size_bytes());

  BinaryChrInfo info{
      .name = name,
      .sourcePath = sourcePath,
      .frameDuration = frameDuration,
      .materialCount = static_cast<u16>(nodes.size()),
      .wrapMode = wrapMode,
	  .scaleRule = scaleRule,
  };
  info.write(writer, names, start);

  BetterDictionary dict;

  std::vector<u32> track_addresses;
  // Edge case: no root node if 1 entry
  auto dictSize = CalcDictionarySize(nodes.size());
  u32 accum = start + 0x2c /* header */ + dictSize;
  for (auto& mat : nodes) {
    dict.nodes.push_back(BetterNode{.name = mat.name, .stream_pos = accum});
    accum += mat.filesize();
  }
  offsets.ofsMatDict = writer.tell() - start;
  WriteDictionary(dict, writer, names);
  for (auto& node : nodes) {
    node.write(writer, names);
  }
  for (auto& track : tracks) {
    writer.alignTo(0x4);
    track.write(writer);
  }

  auto back = writer.tell();
  writer.seekSet(wb);
  offsets.write(writer);
  writer.seekSet(start + 4);
  writer.write<u32>(back - start);
  writer.seekSet(back);
  writer.alignTo(0x4);
}

} // namespace librii::g3d
