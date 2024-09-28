#pragma once

#include <core/common.h>

#include <oishii/writer/binary_writer.hxx>
#include <rsl/SafeReader.hpp>

#include "AnimIO.hpp"

namespace librii::g3d {

struct CHR0Frame32 {
  // raw
  u32 frame; //: 8;
  // Quantization specified externally
  u32 value; // : 12;
  // S6.5
  s32 slope; //: 12;

  f32 slope_decoded() const { return static_cast<f32>(slope) / 32.0f; }

  bool operator==(const CHR0Frame32&) const = default;

  static Result<CHR0Frame32> read(rsl::SafeReader& reader) {
    u32 x = TRY(reader.U32());

    u32 frame = x >> 24;
    u32 value = (x >> 12) & 0xFFF;
    s32 slope = static_cast<int>(x) & 0xFFF;
    if (slope & 0x800) {
      slope |= ~0xFFF;
    }
    return CHR0Frame32{
        .frame = frame,
        .value = value,
        .slope = slope,
    };
  }
  void write(oishii::Writer& writer) const {
    u32 x = (frame << 24) | (value << 12) | (static_cast<u32>(slope) & 0xFFF);
    writer.write<u32>(x);
  }
};
struct CHR0Frame48 {
  // S10.5
  s16 frame;
  // Quantization specified externally
  u16 value;
  // S7.8
  s16 slope;

  f32 frame_decoded() const { return static_cast<f32>(frame) / 32.0f; }
  f32 slope_decoded() const { return static_cast<f32>(slope) / 256.0f; }

  bool operator==(const CHR0Frame48&) const = default;

  static Result<CHR0Frame48> read(rsl::SafeReader& reader) {
    CHR0Frame48 result;
    result.frame = TRY(reader.S16());
    result.value = TRY(reader.U16());
    result.slope = TRY(reader.S16());
    return result;
  }
  void write(oishii::Writer& writer) const {
    writer.write<s16>(frame);
    writer.write<u16>(value);
    writer.write<s16>(slope);
  }
};
struct CHR0Frame96 {
  f32 frame;
  f32 value;
  f32 slope;

  bool operator==(const CHR0Frame96&) const = default;

  static Result<CHR0Frame96> read(rsl::SafeReader& reader) {
    CHR0Frame96 result;
    result.frame = TRY(reader.F32());
    result.value = TRY(reader.F32());
    result.slope = TRY(reader.F32());
    return result;
  }
  void write(oishii::Writer& writer) const {
    writer.write<f32>(frame);
    writer.write<f32>(value);
    writer.write<f32>(slope);
  }
};
struct CHR0Track32 {
  // frames[i] * scale + offset
  f32 scale;
  f32 offset;
  std::vector<CHR0Frame32> frames;

  bool operator==(const CHR0Track32&) const = default;

  static Result<CHR0Track32> read(rsl::SafeReader& reader, u32 frames) {
    CHR0Track32 result;
    result.scale = TRY(reader.F32());
    result.offset = TRY(reader.F32());
    for (u32 i = 0; i < frames; ++i) {
      result.frames.push_back(TRY(CHR0Frame32::read(reader)));
    }
    return result;
  }
  void write(oishii::Writer& writer) const {
    writer.write<f32>(scale);
    writer.write<f32>(offset);
    for (auto& frame : frames) {
      frame.write(writer);
    }
  }
  u32 filesize() const { return 8 + 4 * frames.size(); }
};
struct CHR0Track48 {
  // frames[i] * scale + offset
  f32 scale;
  f32 offset;
  std::vector<CHR0Frame48> frames;

  bool operator==(const CHR0Track48&) const = default;

  static Result<CHR0Track48> read(rsl::SafeReader& reader, u32 frames) {
    CHR0Track48 result;
    result.scale = TRY(reader.F32());
    result.offset = TRY(reader.F32());
    for (u32 i = 0; i < frames; ++i) {
      result.frames.push_back(TRY(CHR0Frame48::read(reader)));
    }
    return result;
  }
  void write(oishii::Writer& writer) const {
    writer.write<f32>(scale);
    writer.write<f32>(offset);
    for (auto& frame : frames) {
      frame.write(writer);
    }
  }
  u32 filesize() const { return 8 + 6 * frames.size(); }
};
struct CHR0Track96 {
  std::vector<CHR0Frame96> frames;

  bool operator==(const CHR0Track96&) const = default;

  static Result<CHR0Track96> read(rsl::SafeReader& reader, u32 frames) {
    CHR0Track96 result;
    for (u32 i = 0; i < frames; ++i) {
      result.frames.push_back(TRY(CHR0Frame96::read(reader)));
    }
    return result;
  }
  void write(oishii::Writer& writer) const {
    for (auto& frame : frames) {
      frame.write(writer);
    }
  }
  u32 filesize() const { return 12 * frames.size(); }
};
struct CHR0BakedTrack8 {
  // frames[i] * scale + offset
  f32 scale;
  f32 offset;
  std::vector<u8> frames;

  bool operator==(const CHR0BakedTrack8&) const = default;

  static Result<CHR0BakedTrack8> read(rsl::SafeReader& reader, u32 frames) {
    CHR0BakedTrack8 result;
    result.scale = TRY(reader.F32());
    result.offset = TRY(reader.F32());
    for (u32 i = 0; i < frames + 1; ++i) {
      result.frames.push_back(TRY(reader.U8()));
    }
    return result;
  }
  void write(oishii::Writer& writer) const {
    writer.write<f32>(scale);
    writer.write<f32>(offset);
    for (auto& frame : frames) {
      writer.write<u8>(frame);
    }
  }
  u32 filesize() const { return 8 + 1 * frames.size(); }
};
struct CHR0BakedTrack16 {
  // frames[i] * scale + offset
  f32 scale;
  f32 offset;
  std::vector<u16> frames;

  bool operator==(const CHR0BakedTrack16&) const = default;

  static Result<CHR0BakedTrack16> read(rsl::SafeReader& reader, u32 frames) {
    CHR0BakedTrack16 result;
    result.scale = TRY(reader.F32());
    result.offset = TRY(reader.F32());
    for (u32 i = 0; i < frames + 1; ++i) {
      result.frames.push_back(TRY(reader.U16()));
    }
    return result;
  }
  void write(oishii::Writer& writer) const {
    writer.write<f32>(scale);
    writer.write<f32>(offset);
    for (auto& frame : frames) {
      writer.write<u16>(frame);
    }
  }
  u32 filesize() const { return 8 + 2 * frames.size(); }
};
struct CHR0BakedTrack32 {
  std::vector<f32> frames;

  bool operator==(const CHR0BakedTrack32&) const = default;

  static Result<CHR0BakedTrack32> read(rsl::SafeReader& reader, u32 frames) {
    CHR0BakedTrack32 result;
    for (u32 i = 0; i < frames + 1; ++i) {
      result.frames.push_back(TRY(reader.F32()));
    }
    return result;
  }
  void write(oishii::Writer& writer) const {
    for (auto& frame : frames) {
      writer.write<f32>(frame);
    }
  }
  u32 filesize() const { return 4 * frames.size(); }
};
struct CHR0Track {
  f32 step;
  std::variant<CHR0Track32, CHR0Track48, CHR0Track96> data;

  bool operator==(const CHR0Track&) const = default;

  static Result<CHR0Track> read(rsl::SafeReader& reader, u32 fmt) {
    CHR0Track result;

    u16 frames = TRY(reader.U16());
    // PADDING
    TRY(reader.U16());
    result.step = TRY(reader.F32());

    if (fmt == 0) {
      result.data = TRY(CHR0Track32::read(reader, frames));
    } else if (fmt == 1) {
      result.data = TRY(CHR0Track48::read(reader, frames));
    } else if (fmt == 2) {
      result.data = TRY(CHR0Track96::read(reader, frames));
    }

    return result;
  }
  void write(oishii::Writer& writer) const {
    u16 frames = 0;
    if (auto* x = std::get_if<CHR0Track32>(&data)) {
      frames = static_cast<u16>(x->frames.size());
    } else if (auto* x = std::get_if<CHR0Track48>(&data)) {
      frames = static_cast<u16>(x->frames.size());
    } else if (auto* x = std::get_if<CHR0Track96>(&data)) {
      frames = static_cast<u16>(x->frames.size());
    }

    writer.write<u16>(frames);
    writer.write<u16>(0);
    writer.write<f32>(step);

    if (auto* x = std::get_if<CHR0Track32>(&data)) {
      x->write(writer);
    } else if (auto* x = std::get_if<CHR0Track48>(&data)) {
      x->write(writer);
    } else if (auto* x = std::get_if<CHR0Track96>(&data)) {
      x->write(writer);
    }
  }
  u32 filesize() const {
    if (auto* x = std::get_if<CHR0Track32>(&data)) {
      return 8 + x->filesize();
    } else if (auto* x = std::get_if<CHR0Track48>(&data)) {
      return 8 + x->filesize();
    } else if (auto* x = std::get_if<CHR0Track96>(&data)) {
      return 8 + x->filesize();
    }
    assert(!"Internal error");
  }
};
struct CHR0BakedTrack {
  std::variant<CHR0BakedTrack8, CHR0BakedTrack16, CHR0BakedTrack32> data;

  bool operator==(const CHR0BakedTrack&) const = default;

  static Result<CHR0BakedTrack> read(rsl::SafeReader& reader, u32 fmt,
                                     u32 frames) {
    CHR0BakedTrack result;
    if (fmt == 0) {
      result.data = TRY(CHR0BakedTrack8::read(reader, frames));
    } else if (fmt == 1) {
      result.data = TRY(CHR0BakedTrack16::read(reader, frames));
    } else if (fmt == 2) {
      result.data = TRY(CHR0BakedTrack32::read(reader, frames));
    }
    return result;
  }
  void write(oishii::Writer& writer) const {
    if (auto* x = std::get_if<CHR0BakedTrack8>(&data)) {
      x->write(writer);
    } else if (auto* x = std::get_if<CHR0BakedTrack16>(&data)) {
      x->write(writer);
    } else if (auto* x = std::get_if<CHR0BakedTrack32>(&data)) {
      x->write(writer);
    }
  }
  u32 filesize() const {
    if (auto* x = std::get_if<CHR0BakedTrack8>(&data)) {
      return x->filesize();
    } else if (auto* x = std::get_if<CHR0BakedTrack16>(&data)) {
      return x->filesize();
    } else if (auto* x = std::get_if<CHR0BakedTrack32>(&data)) {
      return x->filesize();
    }
    assert(!"Internal error");
  }
};

struct CHR0AnyTrack {
  std::variant<CHR0Track, CHR0BakedTrack> data;

  bool operator==(const CHR0AnyTrack&) const = default;

  static Result<CHR0AnyTrack> read(rsl::SafeReader& reader, bool baked, u32 fmt,
                                   u32 frames) {
    CHR0AnyTrack result;
    if (!baked) {
      result.data = TRY(CHR0Track::read(reader, fmt));
    } else {
      result.data = TRY(CHR0BakedTrack::read(reader, fmt, frames));
    }
    return result;
  }
  void write(oishii::Writer& writer) const {
    if (auto* x = std::get_if<CHR0Track>(&data)) {
      x->write(writer);
    } else if (auto* x = std::get_if<CHR0BakedTrack>(&data)) {
      x->write(writer);
    }
  }
  u32 filesize() const {
    if (auto* x = std::get_if<CHR0Track>(&data)) {
      return x->filesize();
    } else if (auto* x = std::get_if<CHR0BakedTrack>(&data)) {
      return x->filesize();
    }
    assert(!"Internal error");
  }
};

enum class CHR0Attrib {
  SCL_X,
  SCL_Y,
  SCL_Z,
  ROT_X,
  ROT_Y,
  ROT_Z,
  TRANS_X,
  TRANS_Y,
  TRANS_Z,
};
static constexpr std::array<CHR0Attrib, 9> CHR0Attribs = {
    CHR0Attrib::SCL_X,   CHR0Attrib::SCL_Y,   CHR0Attrib::SCL_Z,
    CHR0Attrib::ROT_X,   CHR0Attrib::ROT_Y,   CHR0Attrib::ROT_Z,
    CHR0Attrib::TRANS_X, CHR0Attrib::TRANS_Y, CHR0Attrib::TRANS_Z,
};

struct CHR0Flags {
  enum {
    ENABLED = 1 << 0,
    SRT_IDENTITY = 1 << 1,
    RT_ZERO = 1 << 2,
    SCL_ONE = 1 << 3,
    SCL_ISOTROPIC = 1 << 4,
    ROT_ZERO = 1 << 5,
    TRANS_ZERO = 1 << 6,
    SCL_MODEL = 1 << 7,
    ROT_MODEL = 1 << 8,
    TRANS_MODEL = 1 << 9,
    SSC_APPLY = 1 << 10,
    SSC_PARENT = 1 << 11,
    CLASSIC_SCALE_OFF = 1 << 12,

    SX_CONST = 1 << 13,
    SY_CONST = 1 << 14,
    SZ_CONST = 1 << 15,
    RX_CONST = 1 << 16,
    RY_CONST = 1 << 17,
    RZ_CONST = 1 << 18,
    TX_CONST = 1 << 19,
    TY_CONST = 1 << 20,
    TZ_CONST = 1 << 21,

    REQUIRE_SCALE = 1 << 22,
    REQUIRE_ROT = 1 << 23,
    REQUIRE_TRANS = 1 << 24,

    SCALETYPE_LOW = 1 << 25,
    SCALETYPE_HI = 1 << 26,

    ROTTYPE_LOW = 1 << 27,
    ROTTYPE_MID = 1 << 28,
    ROTTYPE_HIGH = 1 << 29,

    TRANSTYPE_LOW = 1 << 30,
    TRANSTYPE_HI = 1 << 31,

    OMIT_SX = SRT_IDENTITY | SCL_ONE | SCL_MODEL,
    OMIT_SY = SRT_IDENTITY | SCL_ONE | SCL_ISOTROPIC | SCL_MODEL,
    OMIT_SZ = SRT_IDENTITY | SCL_ONE | SCL_ISOTROPIC | SCL_MODEL,
    OMIT_RX = SRT_IDENTITY | RT_ZERO | ROT_ZERO | ROT_MODEL,
    OMIT_RY = SRT_IDENTITY | RT_ZERO | ROT_ZERO | ROT_MODEL,
    OMIT_RZ = SRT_IDENTITY | RT_ZERO | ROT_ZERO | ROT_MODEL,
    OMIT_TX = SRT_IDENTITY | RT_ZERO | TRANS_ZERO | TRANS_MODEL,
    OMIT_TY = SRT_IDENTITY | RT_ZERO | TRANS_ZERO | TRANS_MODEL,
    OMIT_TZ = SRT_IDENTITY | RT_ZERO | TRANS_ZERO | TRANS_MODEL,
  };
  static bool HasAttrib(u32 flags, CHR0Attrib attr) {
    switch (attr) {
    case CHR0Attrib::SCL_X:
      return (flags & OMIT_SX) == 0;
    case CHR0Attrib::SCL_Y:
      return (flags & OMIT_SY) == 0;
    case CHR0Attrib::SCL_Z:
      return (flags & OMIT_SZ) == 0;
    case CHR0Attrib::ROT_X:
      return (flags & OMIT_RX) == 0;
    case CHR0Attrib::ROT_Y:
      return (flags & OMIT_RY) == 0;
    case CHR0Attrib::ROT_Z:
      return (flags & OMIT_RZ) == 0;
    case CHR0Attrib::TRANS_X:
      return (flags & OMIT_TX) == 0;
    case CHR0Attrib::TRANS_Y:
      return (flags & OMIT_TY) == 0;
    case CHR0Attrib::TRANS_Z:
      return (flags & OMIT_TZ) == 0;
    }
    return false;
  }
  static bool IsAttribConst(u32 flags, CHR0Attrib attr) {
    switch (attr) {
    case CHR0Attrib::SCL_X:
      return flags & SX_CONST;
    case CHR0Attrib::SCL_Y:
      return flags & SY_CONST;
    case CHR0Attrib::SCL_Z:
      return flags & SZ_CONST;
    case CHR0Attrib::ROT_X:
      return flags & RX_CONST;
    case CHR0Attrib::ROT_Y:
      return flags & RY_CONST;
    case CHR0Attrib::ROT_Z:
      return flags & RZ_CONST;
    case CHR0Attrib::TRANS_X:
      return flags & TX_CONST;
    case CHR0Attrib::TRANS_Y:
      return flags & TY_CONST;
    case CHR0Attrib::TRANS_Z:
      return flags & TZ_CONST;
    }
    return false;
  }

  enum class ScaleTransFmt {
    Const,
    _32,
    _48,
    _96,
  };

  enum class RotFmt {
    Const,
    _32,
    _48,
    _96,
    Baked8,
    Baked16,
    Baked32,
  };

  static RotFmt ToRotFmt(ScaleTransFmt f) { return static_cast<RotFmt>(f); }

  static ScaleTransFmt ScaleEncoding(u32 flags) {
    return static_cast<ScaleTransFmt>((flags >> 25) & 0b11);
  }
  static RotFmt RotateEncoding(u32 flags) {
    return static_cast<RotFmt>((flags >> 27) & 0b111);
  }
  static ScaleTransFmt TranslateEncoding(u32 flags) {
    return static_cast<ScaleTransFmt>((flags >> 30) & 0b11);
  }
};

struct CHR0Node {
  std::string name;
  u32 flags;
  // index or const
  std::vector<std::variant<u32, f32>> tracks;

  bool operator==(const CHR0Node&) const = default;

  static Result<CHR0Node> read(rsl::SafeReader& reader,
                               std::map<u32, CHR0Flags::RotFmt>& tracks) {
    u32 start = reader.tell();
    CHR0Node result;
    result.name = TRY(reader.StringOfs32(reader.tell()));
    result.flags = TRY(reader.U32());
    for (auto attr : CHR0Attribs) {
      if (!CHR0Flags::HasAttrib(result.flags, attr))
        continue;

      // RotFmt is a uperset of scale/trans
      CHR0Flags::RotFmt fmt = CHR0Flags::RotFmt::Const;
      switch (attr) {
      case CHR0Attrib::SCL_X:
      case CHR0Attrib::SCL_Y:
      case CHR0Attrib::SCL_Z:
        fmt = CHR0Flags::ToRotFmt(CHR0Flags::ScaleEncoding(result.flags));
        break;
      case CHR0Attrib::ROT_X:
      case CHR0Attrib::ROT_Y:
      case CHR0Attrib::ROT_Z:
        fmt = CHR0Flags::RotateEncoding(result.flags);
        break;
      case CHR0Attrib::TRANS_X:
      case CHR0Attrib::TRANS_Y:
      case CHR0Attrib::TRANS_Z:
        fmt = CHR0Flags::ToRotFmt(CHR0Flags::TranslateEncoding(result.flags));
        break;
      }

      // E.g. only XZ specified
      if (CHR0Flags::IsAttribConst(result.flags, attr)) {
        fmt = CHR0Flags::RotFmt::Const;
      }

      if (fmt == CHR0Flags::RotFmt::Const) {
        result.tracks.push_back(TRY(reader.F32()));
      } else {
        u32 ofs = start + TRY(reader.S32());
        if (tracks.contains(ofs)) {
          EXPECT(tracks.at(ofs) == fmt &&
                 "Type-punning of track data is unsupported");
        }
        tracks[ofs] = fmt;
        result.tracks.push_back(ofs);
      }
    }
    return result;
  }
  u32 filesize() const {
    u32 numTracks = tracks.size();
    return 8 + numTracks * 4;
  }
  void write(oishii::Writer& writer, NameTable& names) const {
    auto start = writer.tell();
    writeNameForward(names, writer, start, name, true);
    writer.write<u32>(flags);
    for (auto& track : tracks) {
      if (auto* c = std::get_if<f32>(&track)) {
        writer.write<f32>(*c);
      } else if (auto* index = std::get_if<u32>(&track)) {
        writer.write<u32>(*index - start);
      } else {
        assert(!"Internal error");
      }
    }
  }
};

struct BinaryChr {
  std::vector<CHR0Node> nodes;
  std::vector<CHR0AnyTrack> tracks;
  // TODO: User data
  std::string name;
  std::string sourcePath;
  u16 frameDuration{};
  AnimationWrapMode wrapMode{AnimationWrapMode::Repeat};
  u32 scaleRule = 0;

  bool operator==(const BinaryChr&) const = default;

  Result<void> read(oishii::BinaryReader& reader);
  void write(oishii::Writer& writer, NameTable& names, u32 addrBrres) const;

  void mergeIdenticalTracks();
};

// Higher-level structure

enum class ChrQuantization {
  Track32,
  Track48,
  Track96,
  BakedTrack8,
  BakedTrack16,
  BakedTrack32,
  Const,
};

struct ChrFrame {
  // We actually need 64 bits of precision currently to get a match. We should
  // be able to finesse byte-matching output while keeping f32 output given the
  // data was originally 32-bit precision?
  f64 frame, value, slope;
  bool operator==(const ChrFrame&) const = default;
};

struct ChrTrack {
  ChrQuantization quant{ChrQuantization::Track96};
  // These are only quantization settings. Do not consider scale/offset when
  // interpreting `frames`
  float scale{1.0f};
  float offset{0.0f};

  std::vector<ChrFrame> frames;

  bool operator==(const ChrTrack&) const = default;

  static ChrTrack from(const CHR0Track32& track);
  static ChrTrack from(const CHR0Track48& track);
  static ChrTrack from(const CHR0Track96& track);
  static ChrTrack from(const CHR0BakedTrack8& track);
  static ChrTrack from(const CHR0BakedTrack16& track);
  static ChrTrack from(const CHR0BakedTrack32& track);
  static ChrTrack from(f32 const_value);

  static ChrTrack
  fromVariant(std::variant<CHR0Track32, CHR0Track48, CHR0Track96> variant);
  static ChrTrack
  fromVariant(std::variant<CHR0BakedTrack8, CHR0BakedTrack16, CHR0BakedTrack32>
                  variant);

  static Result<ChrTrack> fromAny(const CHR0AnyTrack& any,
                                  std::function<void(std::string_view)> warn);

  std::variant<CHR0Track32, CHR0Track48, CHR0Track96, CHR0BakedTrack8,
               CHR0BakedTrack16, CHR0BakedTrack32>
  to() const;
  std::variant<CHR0AnyTrack, f32> to_any() const;
};

struct ChrNode {
  std::string name;
  u32 flags;
  // All indices
  std::vector<u32> tracks;

  bool operator==(const ChrNode&) const = default;
};

struct ChrAnim {
  std::vector<ChrNode> nodes;
  // All existing tracks + extra tracks (to be trimmed) for Const values.
  std::vector<ChrTrack> tracks;
  // TODO: User data
  std::string name;
  std::string sourcePath;
  u16 frameDuration{};
  AnimationWrapMode wrapMode{AnimationWrapMode::Repeat};
  u32 scaleRule = 0;

  static Result<ChrAnim> from(const BinaryChr& binaryChr,
                              std::function<void(std::string_view)> warn);
  BinaryChr to() const;

  Result<void> read(oishii::BinaryReader& reader,
                    std::function<void(std::string_view)> warn);
  void write(oishii::Writer& writer, NameTable& names, u32 addrBrres) const;

  bool operator==(const ChrAnim&) const = default;
};

} // namespace librii::g3d
