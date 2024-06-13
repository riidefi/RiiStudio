#include "AnimChrIO.hpp"

#include <rsl/CompactVector.hpp>

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
    for (auto& n : nodes) {
      for (auto& t : n.tracks) {
        if (auto* o = std::get_if<u32>(&t)) {
          if (*o == ofs) {
            *o = static_cast<u32>(tracks.size() - 1);
          }
        }
      }
    }
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
  auto dictSize = CalcDictionarySize(nodes.size());
  u32 accum = start + 0x2c /* header */ + dictSize;
  for (auto& mat : nodes) {
    dict.nodes.push_back(BetterNode{.name = mat.name, .stream_pos = accum});
    accum += mat.filesize();
  }
  for (auto& track : tracks) {
    accum = roundUp(accum, 4);
    track_addresses.push_back(accum);
    accum += track.filesize();
  }
  offsets.ofsMatDict = writer.tell() - start;
  WriteDictionary(dict, writer, names);

  std::vector<CHR0Node> nodes_tmp = nodes;
  // Convert indices -> offsets
  for (auto& n : nodes_tmp) {
    for (auto& t : n.tracks) {
      if (auto* o = std::get_if<u32>(&t)) {
        assert(*o < track_addresses.size());
        *o = track_addresses[*o];
      }
    }
  }

  for (auto& node : nodes_tmp) {
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

void BinaryChr::mergeIdenticalTracks() {
  auto compacted = rsl::StableCompactVector(tracks);

  // Replace old track indices with new indices in CHR0Node targets
  for (auto& node : nodes) {
    for (auto& target : node.tracks) {
      if (auto* index = std::get_if<u32>(&target)) {
        *index = compacted.remapTableOldToNew[*index];
      }
    }
  }

  // Replace the old tracks with the new list of unique tracks
  tracks = std::move(compacted.uniqueElements);
}

ChrTrack ChrTrack::from(const CHR0Track32& track) {
  ChrTrack chrTrack;
  chrTrack.quant = ChrQuantization::Track32;
  chrTrack.scale = track.scale;
  chrTrack.offset = track.offset;
  for (const auto& frame : track.frames) {
    ChrFrame chrFrame;
    chrFrame.frame = static_cast<float>(frame.frame);
    // Adjusted by scale and offset
    chrFrame.value =
        static_cast<f64>(frame.value) * static_cast<f64>(track.scale) +
        static_cast<f64>(track.offset);
    chrFrame.slope = frame.slope_decoded();
    chrTrack.frames.push_back(chrFrame);
  }
  return chrTrack;
}
ChrTrack ChrTrack::from(const CHR0Track48& track) {
  ChrTrack chrTrack;
  chrTrack.quant = ChrQuantization::Track48;
  chrTrack.scale = track.scale;
  chrTrack.offset = track.offset;
  for (const auto& frame : track.frames) {
    ChrFrame chrFrame;
    chrFrame.frame = frame.frame_decoded();
    // Adjusted by scale and offset
    chrFrame.value =
        static_cast<f64>(frame.value) * static_cast<f64>(track.scale) +
        static_cast<f64>(track.offset);
    chrFrame.slope = frame.slope_decoded();
    chrTrack.frames.push_back(chrFrame);
  }
  return chrTrack;
}
ChrTrack ChrTrack::from(const CHR0Track96& track) {
  ChrTrack chrTrack;
  chrTrack.quant = ChrQuantization::Track96;
  for (const auto& frame : track.frames) {
    ChrFrame chrFrame;
    chrFrame.frame = frame.frame;
    chrFrame.value = frame.value;
    chrFrame.slope = frame.slope;
    chrTrack.frames.push_back(chrFrame);
  }
  return chrTrack;
}
ChrTrack ChrTrack::from(const CHR0BakedTrack8& track) {
  ChrTrack chrTrack;
  chrTrack.quant = ChrQuantization::BakedTrack8;
  chrTrack.scale = track.scale;
  chrTrack.offset = track.offset;
  for (const auto& frame : track.frames) {
    ChrFrame chrFrame;
    chrFrame.frame = 0.f; // Baked tracks do not have separate frame values
    chrFrame.value = frame * track.scale + track.offset;
    chrFrame.slope = 0.f; // No slope for baked tracks
    chrTrack.frames.push_back(chrFrame);
  }
  return chrTrack;
}
ChrTrack ChrTrack::from(const CHR0BakedTrack16& track) {
  ChrTrack chrTrack;
  chrTrack.quant = ChrQuantization::BakedTrack16;
  chrTrack.scale = track.scale;
  chrTrack.offset = track.offset;
  for (const auto& frame : track.frames) {
    ChrFrame chrFrame;
    chrFrame.frame = 0.f; // Baked tracks do not have separate frame values
    chrFrame.value = frame * track.scale + track.offset;
    chrFrame.slope = 0.f; // No slope for baked tracks
    chrTrack.frames.push_back(chrFrame);
  }
  return chrTrack;
}
ChrTrack ChrTrack::from(const CHR0BakedTrack32& track) {
  ChrTrack chrTrack;
  chrTrack.quant = ChrQuantization::BakedTrack32;
  for (const auto& frame : track.frames) {
    ChrFrame chrFrame;
    chrFrame.frame = 0.f; // Baked tracks do not have separate frame values
    chrFrame.value = frame;
    chrFrame.slope = 0.f; // No slope for baked tracks
    chrTrack.frames.push_back(chrFrame);
  }
  return chrTrack;
}
ChrTrack ChrTrack::from(f32 const_value) {
  ChrTrack result;
  result.quant = ChrQuantization::Const;
  result.frames.push_back(ChrFrame{
      .frame = 0.0f,
      .value = const_value,
      .slope = 0.0f,
  });
  return result;
}

ChrTrack ChrTrack::fromVariant(
    std::variant<CHR0Track32, CHR0Track48, CHR0Track96> variant) {
  return std::visit([](const auto& track) -> ChrTrack { return from(track); },
                    variant);
}
ChrTrack ChrTrack::fromVariant(
    std::variant<CHR0BakedTrack8, CHR0BakedTrack16, CHR0BakedTrack32> variant) {
  return std::visit([](const auto& track) -> ChrTrack { return from(track); },
                    variant);
}

ChrTrack ChrTrack::fromAny(const CHR0AnyTrack& any) {
  if (auto* x = std::get_if<CHR0Track>(&any.data)) {
    auto res = fromVariant(x->data);
    res.step = x->step;
    return res;
  } else if (auto* x = std::get_if<CHR0BakedTrack>(&any.data)) {
    return fromVariant(x->data);
  }
}

std::variant<CHR0Track32, CHR0Track48, CHR0Track96, CHR0BakedTrack8,
             CHR0BakedTrack16, CHR0BakedTrack32>
ChrTrack::to() const {
  switch (quant) {
  case ChrQuantization::Track32: {
    CHR0Track32 track;
    track.scale = scale;
    track.offset = offset;
    for (const auto& frame : frames) {
      CHR0Frame32 chrFrame32;
      chrFrame32.frame = static_cast<u32>(roundf(frame.frame));
      chrFrame32.value =
          static_cast<u32>(roundf((frame.value - offset) / scale));
      chrFrame32.slope = static_cast<s32>(roundf(frame.slope * 32.0f));
      track.frames.push_back(chrFrame32);
    }
    return track;
  }
  case ChrQuantization::Track48: {
    CHR0Track48 track;
    track.scale = scale;
    track.offset = offset;
    for (const auto& frame : frames) {
      CHR0Frame48 chrFrame48;
      chrFrame48.frame = static_cast<s16>(roundf(frame.frame * 32.0f));
      f64 f = (static_cast<f64>(frame.value) - offset) / scale;
      chrFrame48.value = static_cast<u16>(round(f));
      chrFrame48.slope = static_cast<s16>(roundf(frame.slope * 256.0f));
      track.frames.push_back(chrFrame48);
    }
    return track;
  }
  case ChrQuantization::Track96: {
    CHR0Track96 track;
    for (const auto& frame : frames) {
      CHR0Frame96 chrFrame96;
      chrFrame96.frame = frame.frame;
      chrFrame96.value = frame.value;
      chrFrame96.slope = frame.slope;
      track.frames.push_back(chrFrame96);
    }
    return track;
  }
  case ChrQuantization::BakedTrack8: {
    CHR0BakedTrack8 track;
    track.scale = scale;
    track.offset = offset;
    for (const auto& frame : frames) {
      track.frames.push_back(
          static_cast<u8>(roundf((frame.value - offset) / scale)));
    }
    return track;
  }
  case ChrQuantization::BakedTrack16: {
    CHR0BakedTrack16 track;
    track.scale = scale;
    track.offset = offset;
    for (const auto& frame : frames) {
      track.frames.push_back(
          static_cast<u16>(roundf((frame.value - offset) / scale)));
    }
    return track;
  }
  case ChrQuantization::BakedTrack32: {
    CHR0BakedTrack32 track;
    for (const auto& frame : frames) {
      track.frames.push_back(frame.value);
    }
    return track;
  }
  }
}

std::variant<CHR0AnyTrack, f32> ChrTrack::to_any() const {
  float step = frames.size() >= 2
                   ? CalcStep(frames.front().frame, frames.back().frame)
                   : 0.0f;

  step = this->step;

  // Determine the appropriate CHR0Track or CHR0BakedTrack variant
  CHR0Track chr0Track;
  CHR0BakedTrack chr0BakedTrack;

  switch (quant) {
  case ChrQuantization::Track32: {
    chr0Track.step = step;
    chr0Track.data = std::get<CHR0Track32>(to());
    break;
  }
  case ChrQuantization::Track48: {
    chr0Track.step = step;
    chr0Track.data = std::get<CHR0Track48>(to());
    break;
  }
  case ChrQuantization::Track96: {
    chr0Track.step = step;
    chr0Track.data = std::get<CHR0Track96>(to());
    break;
  }
  case ChrQuantization::BakedTrack8: {
    chr0BakedTrack.data = std::get<CHR0BakedTrack8>(to());
    break;
  }
  case ChrQuantization::BakedTrack16: {
    chr0BakedTrack.data = std::get<CHR0BakedTrack16>(to());
    break;
  }
  case ChrQuantization::BakedTrack32: {
    chr0BakedTrack.data = std::get<CHR0BakedTrack32>(to());
    break;
  }
  case ChrQuantization::Const:
    break;
  }

  if (quant == ChrQuantization::Track32 || quant == ChrQuantization::Track48 ||
      quant == ChrQuantization::Track96) {
    return CHR0AnyTrack{chr0Track};
  } else if (quant == ChrQuantization::Const) {
    assert(frames.size() >= 1);
    return static_cast<f32>(frames[0].value);
  } else {
    return CHR0AnyTrack{chr0BakedTrack};
  }
}

ChrAnim ChrAnim::from(const BinaryChr& binaryChr) {
  ChrAnim anim;
  anim.name = binaryChr.name;
  anim.sourcePath = binaryChr.sourcePath;
  anim.frameDuration = binaryChr.frameDuration;
  anim.wrapMode = binaryChr.wrapMode;
  anim.scaleRule = binaryChr.scaleRule;

  // Add tracks first
  for (const auto& track : binaryChr.tracks) {
    anim.tracks.push_back(ChrTrack::fromAny(track));
  }

  // Add nodes with track indices
  for (const auto& node : binaryChr.nodes) {
    ChrNode chrNode;
    chrNode.name = node.name;
    chrNode.flags = node.flags;
    for (const auto& track : node.tracks) {
      if (std::holds_alternative<u32>(track)) {
        chrNode.tracks.push_back(std::get<u32>(track));
      } else {
        f32 const_value = std::get<f32>(track);
        // Add a new Const track
        ChrTrack constTrack = ChrTrack::from(const_value);
        anim.tracks.push_back(constTrack);
        chrNode.tracks.push_back(anim.tracks.size() - 1);
      }
    }
    anim.nodes.push_back(chrNode);
  }

  return anim;
}

BinaryChr ChrAnim::to() const {
  BinaryChr binaryChr;

  // Copy metadata
  binaryChr.name = name;
  binaryChr.sourcePath = sourcePath;
  binaryChr.frameDuration = frameDuration;
  binaryChr.wrapMode = wrapMode;
  binaryChr.scaleRule = scaleRule;

  // Convert and add tracks

  // NOTE: ASSUMES CONST TRACKS ARE AT THE END!

  bool is_const_mode = false;
  for (const auto& track : tracks) {
    if (track.quant == ChrQuantization::Const) {
      is_const_mode = true;
      continue;
    }
    if (is_const_mode) {
      assert(false && "Invalid track ordering. CONST tracks must be "
                      "contiguous and final");
      exit(1);
    }
    binaryChr.tracks.push_back(std::get<CHR0AnyTrack>(track.to_any()));
  }

  // Convert and add nodes
  for (const auto& node : nodes) {
    CHR0Node chr0Node;
    chr0Node.name = node.name;
    chr0Node.flags = node.flags;

    for (const auto& trackIndex : node.tracks) {
      if (trackIndex >= tracks.size()) {
        assert(false && "Track index out of bounds");
        exit(1);
      }
      const auto& track = tracks[trackIndex];
      if (track.quant == ChrQuantization::Const) {
        chr0Node.tracks.push_back(std::get<f32>(track.to_any()));
      } else {
        chr0Node.tracks.push_back(trackIndex);
      }
    }

    binaryChr.nodes.push_back(chr0Node);
  }

  return binaryChr;
}

Result<void> ChrAnim::read(oishii::BinaryReader& reader) {
  BinaryChr tmp;
  TRY(tmp.read(reader));
  *this = ChrAnim::from(tmp);

#if 0
  auto checked_tmp = to();
  assert(tmp.tracks.size() == checked_tmp.tracks.size());
  if (tmp.tracks.size() == checked_tmp.tracks.size()) {
    for (int i = 0; i < checked_tmp.tracks.size(); ++i) {
      CHR0AnyTrack good_track = tmp.tracks[i];
      CHR0AnyTrack bad_track = checked_tmp.tracks[i];

      assert(good_track == bad_track);
    }
  }
  assert(tmp.tracks == checked_tmp.tracks);
  assert(tmp.nodes.size() == checked_tmp.nodes.size());
  if (tmp.nodes.size() == checked_tmp.nodes.size()) {
    for (int i = 0; i < checked_tmp.nodes.size(); ++i) {
      auto& good = tmp.nodes[i];
      auto& bad = checked_tmp.nodes[i];

      assert(good == bad);
    }
  }
  assert(tmp.nodes == checked_tmp.nodes);
  assert(tmp == checked_tmp);
#endif

  return {};
}
void ChrAnim::write(oishii::Writer& writer, NameTable& names,
                    u32 addrBrres) const {
  auto bin = to();
  bin.write(writer, names, addrBrres);
}

} // namespace librii::g3d
