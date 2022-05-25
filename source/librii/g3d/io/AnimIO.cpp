#include "AnimIO.hpp"
#include "DictWriteIO.hpp"
#include "librii/g3d/data/AnimData.hpp"
#include "librii/g3d/io/CommonIO.hpp"
#include "librii/g3d/io/DictIO.hpp"
#include <algorithm>
#include <map>
#include <rsl/SimpleReader.hpp>
#include <span>
#include <string_view>

namespace librii::g3d {

#pragma region Key Frame : Fixed

constexpr u32 KeyFrameBinSize = 12;

static void ReadKeyFrame(KeyFrame& out, std::span<const u8> data) {
  assert(data.size_bytes() >= KeyFrameBinSize);

  out.frame = rsl::load<f32>(data, 0);
  out.value = rsl::load<f32>(data, 4);
  out.slope = rsl::load<f32>(data, 8);
}

static void WriteKeyFrame(std::span<u8> data, const KeyFrame& in) {
  assert(data.size_bytes() >= KeyFrameBinSize);

  rsl::store<f32>(in.frame, data, 0);
  rsl::store<f32>(in.value, data, 4);
  rsl::store<f32>(in.slope, data, 8);
}

#pragma endregion

#pragma region Key Frame Collection : Variable
static bool ReadKeyFrameCollection(KeyFrameCollection& out,
                                   std::span<const u8> data) {
  constexpr u32 header_size = 8;
  if (data.size_bytes() < header_size)
    return false;

  const u16 keyframe_count = rsl::load<u16>(data, 0);
  const u32 body_size = keyframe_count * KeyFrameBinSize;

  out.step = rsl::load<f32>(data, 4);

  const u32 required_size = header_size + body_size;
  if (data.size_bytes() < required_size)
    return false;

  out.data.resize(keyframe_count);

  for (size_t i = 0; i < out.data.size(); ++i) {
    ReadKeyFrame(out.data[i], data.subspan(header_size + i * KeyFrameBinSize,
                                           KeyFrameBinSize));
  }

  // TODO: Warn if out.step != CalcStep

  return true;
}

[[maybe_unused]] static u32 CalcKeyFrameCollectionSize(u32 num_keys) {
  constexpr u32 header_size = 8;

  return header_size + KeyFrameBinSize * num_keys;
}

[[maybe_unused]] static void
WriteKeyFrameCollection(std::span<u8> data, const KeyFrameCollection& in) {
  assert(data.size_bytes() >= CalcKeyFrameCollectionSize(in.data.size()));
  rsl::store<u16>(in.data.size(), data, 0);
  rsl::store<u16>(0, data, 2); // Padding
  rsl::store<f32>(in.step, data, 4);

  for (auto it = in.data.begin(); it != in.data.end(); ++it) {
    const size_t index = std::distance(in.data.begin(), it);

    WriteKeyFrame(data.subspan(8 + index * KeyFrameBinSize, KeyFrameBinSize),
                  *it);
  }
}
#pragma endregion

#pragma region Temporal Wrap Mode : Fixed
static AnimationWrapMode ReadAnimationWrapMode(std::span<const u8> data) {
  assert(data.size_bytes() >= 4);

  return static_cast<AnimationWrapMode>(rsl::load<u32>(data, 0));
}

static void WriteAnimationWrapMode(std::span<u8> data, AnimationWrapMode mode) {
  assert(data.size_bytes() >= 4);

  rsl::store<u32>(static_cast<u32>(mode), data, 0);
}
#pragma endregion

#pragma region SRT Control : Fixed
struct SrtControl {
  u16 frame_duration;
  u16 material_count;
  u32 xform_model;
  AnimationWrapMode wrap_mode;
};

constexpr u32 SrtControlBinSize = 12;

static SrtControl ReadSrtControl(std::span<const u8> data) {
  assert(data.size_bytes() >= SrtControlBinSize);

  return SrtControl{.frame_duration = rsl::load<u16>(data, 0),
                    .material_count = rsl::load<u16>(data, 2),
                    .xform_model = rsl::load<u32>(data, 4),
                    .wrap_mode = ReadAnimationWrapMode(data.subspan(8))};
}

[[maybe_unused]] static void WriteSrtControl(std::span<u8> data,
                                             const SrtControl& control) {
  assert(data.size_bytes() >= SrtControlBinSize);

  rsl::store<u16>(control.frame_duration, data, 0);
  rsl::store<u16>(control.material_count, data, 2);

  rsl::store<u32>(control.xform_model, data, 4);
  WriteAnimationWrapMode(data.subspan(8), control.wrap_mode);
}
#pragma endregion

#pragma region Raw SRT Flag Bitfield : Data
enum class SrtTexFlagVals {
  Unknown,
  ScaleOne,
  RotZero,
  TransZero,
  ScaleIsotropic,
  ScaleUFixed,
  ScaleVFixed,
  RotFixed,
  TransUFixed,
  TransVFixed
};

static bool TestSrtTexFlag(u32 flags, SrtTexFlagVals val) {
  const u32 flag_bit = 1 << static_cast<u32>(val);

  return flags & flag_bit;
}

static u32 SetSrtTexFlag(u32 flags, SrtTexFlagVals val) {
  const u32 flag_bit = 1 << static_cast<u32>(val);

  return flags | flag_bit;
}
#pragma endregion

#pragma region Raw SRT Tex Header
struct SrtTexDataHeader {
  u32 flags; // SrtTexFlagVals
};

constexpr u32 SrtTexDataHeaderBinSize = 4;

static SrtTexDataHeader ReadSrtTexDataHeader(std::span<const u8> data) {
  assert(data.size_bytes() >= SrtTexDataHeaderBinSize);

  return SrtTexDataHeader{.flags = rsl::load<u32>(data, 0)};
}

[[maybe_unused]] static void
WriteSrtTexDataHeader(std::span<u8> data, const SrtTexDataHeader& header) {
  assert(data.size_bytes() >= SrtTexDataHeaderBinSize);

  rsl::store<u32>(header.flags, data, 0);
}
#pragma endregion

// TODO: Need writing

#pragma region Decomposed SRT Tex Header : Fixed
SrtFlags ConvertToFlags(const SrtTexDataHeader& header) {
  return SrtFlags{
      .Unknown = TestSrtTexFlag(header.flags, SrtTexFlagVals::Unknown),
      .ScaleOne = TestSrtTexFlag(header.flags, SrtTexFlagVals::ScaleOne),
      .RotZero = TestSrtTexFlag(header.flags, SrtTexFlagVals::RotZero),
      .TransZero = TestSrtTexFlag(header.flags, SrtTexFlagVals::TransZero),
      .ScaleUniform =
          TestSrtTexFlag(header.flags, SrtTexFlagVals::ScaleIsotropic)};
}

SrtTexDataHeader ConvertFromFlags(const SrtFlags& flags) {
  u32 result = 0;

  if (flags.Unknown)
    result = SetSrtTexFlag(result, SrtTexFlagVals::Unknown);
  if (flags.ScaleOne)
    result = SetSrtTexFlag(result, SrtTexFlagVals::ScaleOne);
  if (flags.RotZero)
    result = SetSrtTexFlag(result, SrtTexFlagVals::RotZero);
  if (flags.TransZero)
    result = SetSrtTexFlag(result, SrtTexFlagVals::TransZero);
  if (flags.ScaleUniform)
    result = SetSrtTexFlag(result, SrtTexFlagVals::ScaleIsotropic);

  return SrtTexDataHeader{.flags = result};
}

std::array<bool, NumberOfSrtAttributes>
ConvertToAttrArray(const SrtTexDataHeader& header) {
  return {TestSrtTexFlag(header.flags, SrtTexFlagVals::ScaleUFixed),
          TestSrtTexFlag(header.flags, SrtTexFlagVals::ScaleVFixed),
          TestSrtTexFlag(header.flags, SrtTexFlagVals::RotFixed),
          TestSrtTexFlag(header.flags, SrtTexFlagVals::TransUFixed),
          TestSrtTexFlag(header.flags, SrtTexFlagVals::TransVFixed)};
}

struct SrtTexData {
  SrtFlags flags;
  std::array<bool, NumberOfSrtAttributes>
      attributes_fixed; // True = fixed, false = animated
};

std::optional<SrtTexData> ReadSrtTexDataFixed(std::span<const u8> whole_data,
                                              unsigned offset) {
  if (whole_data.subspan(offset).size_bytes() < SrtTexDataHeaderBinSize) {
    return std::nullopt;
  }

  const auto header = ReadSrtTexDataHeader(whole_data.subspan(offset));

  return SrtTexData{.flags = ConvertToFlags(header),
                    .attributes_fixed = ConvertToAttrArray(header)};
}
#pragma endregion

// TODO: Need writing
#pragma region SRT Tex Header + Data pointers : Variable
struct SrtTexDataEx {
  SrtTexData data;
  std::array<std::optional<s32>, NumberOfSrtAttributes> anim_handles_offsets;
};

static u32 CalcNumSrtTexDataAnimProperties(const SrtFlags& flags) {
  u32 counter = 0;

  for (int i = 0; i < NumberOfSrtAttributes; ++i) {
    const SrtAttribute attrib = static_cast<SrtAttribute>(i);

    if (!IsSrtAttributeIncluded(flags, attrib))
      ++counter;
  }

  return counter;
}

[[maybe_unused]] static u32 CalcSrtTexDataSize(const SrtTexDataEx& data) {
  return SrtTexDataHeaderBinSize +
         sizeof(s32) * CalcNumSrtTexDataAnimProperties(data.data.flags);
}

static std::optional<SrtTexDataEx>
ReadSrtTexData(std::span<const u8> whole_data, unsigned offset) {
  const auto header = ReadSrtTexDataFixed(whole_data, offset);
  if (!header.has_value()) {
    return std::nullopt;
  }

  if (whole_data.subspan(offset).size_bytes() <
      CalcNumSrtTexDataAnimProperties(header->flags)) {
    return std::nullopt;
  }

  SrtTexDataEx result;
  result.data = *header;

  // TODO: Make own function
  unsigned cursor = offset + SrtTexDataHeaderBinSize;

  for (int i = 0; i < NumberOfSrtAttributes; ++i) {
    const SrtAttribute attrib = static_cast<SrtAttribute>(i);

    if (!IsSrtAttributeIncluded(header->flags, attrib))
      continue;

    if (cursor >= whole_data.size_bytes())
      return std::nullopt;

    result.anim_handles_offsets[i] = cursor;
    cursor += 4;
  }

  return result;
}

#pragma endregion

// TODO: Need writing
#pragma region SRT Header : Fixed
struct SrtHeader {
  CommonHeader common;
  s32 ofs_srt_dict;
  s32 ofs_user;

  std::string_view name;
  std::string_view source;

  SrtControl control;
};

constexpr u32 SrtHeaderBinSize = CommonHeaderBinSize /* common */ +
                                 16 /* ofs_srt_dict..source */ +
                                 SrtControlBinSize /* control */;

static std::optional<SrtHeader> ReadSrtHeader(std::span<const u8> data) {
  if (data.size_bytes() < SrtHeaderBinSize) {
    return std::nullopt;
  }

  const auto common = ReadCommonHeader(data);

  if (!common.has_value()) {
    return std::nullopt;
  }

  return SrtHeader{
      .common = common.value(),
      .ofs_srt_dict = rsl::load<s32>(data, CommonHeaderBinSize),
      .ofs_user = rsl::load<s32>(data, CommonHeaderBinSize + 4),
      .name = ReadStringPointer(data, CommonHeaderBinSize + 8, 0),
      .source = ReadStringPointer(data, CommonHeaderBinSize + 12, 0),
      .control = ReadSrtControl(data.subspan(CommonHeaderBinSize + 16))};
}

#pragma endregion

// TODO: Need writing
#pragma region SRT Material Header : Fixed
struct SrtMatDataHeader {
  std::string_view material_name;
  u32 enabled_texsrts = 0;
  u32 enabled_indmtxs = 0;
};

constexpr u32 SrtMatDataHeaderBinSize = 12;

SrtMatDataHeader ReadSrtMatDataHeader(std::span<const u8> data) {
  assert(data.size_bytes() >= SrtMatDataHeaderBinSize);
  return SrtMatDataHeader{.material_name = ReadStringPointer(data, 0, 0),
                          .enabled_texsrts = rsl::load<u32>(data, 4),
                          .enabled_indmtxs = rsl::load<u32>(data, 8)};
}

constexpr bool IsTexAnimated(const SrtMatDataHeader& header, int id) {
  assert(id >= 0 && id < 8);
  return header.enabled_texsrts & (1 << id);
}

constexpr bool IsIndAnimated(const SrtMatDataHeader& header, int id) {
  assert(id >= 0 && id < 3);
  return header.enabled_indmtxs & (1 << id);
}

constexpr size_t CalcNumAnimations(const SrtMatDataHeader& header) {
  int total = 0;

  for (int i = 0; i < 8; ++i)
    total += IsTexAnimated(header, i) ? 1 : 0;

  for (int i = 0; i < 3; ++i)
    total += IsIndAnimated(header, i) ? 1 : 0;

  return total;
}
#pragma endregion

// TODO: Need writing
#pragma region SRT Material Data : Variable
struct SrtMatData {
  std::string_view material_name;

  std::array<std::optional<s32>, 8> abs_tex_ofs;
  std::array<std::optional<s32>, 3> abs_ind_ofs;
};

static u32 CalcSrtMatDataSize(const SrtMatDataHeader& header) {
  return SrtMatDataHeaderBinSize + CalcNumAnimations(header) * sizeof(s32);
}

std::optional<SrtMatData> ReadSrtMatData(std::span<const u8> whole_data,
                                         size_t offset) {
  auto data = whole_data.subspan(offset);

  if (data.size_bytes() < SrtMatDataHeaderBinSize) {
    return std::nullopt;
  }

  const auto header = ReadSrtMatDataHeader(data);

  if (data.size_bytes() < CalcSrtMatDataSize(header)) {
    return std::nullopt;
  }

  SrtMatData result{.material_name = header.material_name};

  unsigned cursor = SrtMatDataHeaderBinSize;

  for (int i = 0; i < 8; ++i) {
    if (IsTexAnimated(header, i)) {
      result.abs_tex_ofs[i] = offset + rsl::load_update<s32>(data, cursor);
    }
  }
  for (int i = 0; i < 3; ++i) {
    if (IsIndAnimated(header, i)) {
      result.abs_ind_ofs[i] = offset + rsl::load_update<s32>(data, cursor);
    }
  }

  return result;
}

#pragma endregion

// TODO: Need writing
// TODO: Need size calc
#pragma region Texture Anim : Data->(Fixed | Variable)
static std::optional<SrtMatrixAnimation>
ReadTexSrtAnimation(std::span<const u8> data, unsigned texsrt_offset) {
  auto tex_desc = ReadSrtTexData(data, texsrt_offset);
  if (!tex_desc.has_value()) {
    return std::nullopt;
  }

  SrtMatrixAnimation tex_out;
  tex_out.flags = tex_desc->data.flags;

  for (int i = 0; i < NumberOfSrtAttributes; ++i) {
    if (!tex_desc->anim_handles_offsets[i].has_value())
      continue;

    if (tex_desc->data.attributes_fixed[i]) {
      const f32 fixed_val =
          rsl::load<f32>(data, *tex_desc->anim_handles_offsets[i]);

      tex_out.animations[i] = Animation(FixedAnimation{.value = fixed_val});
    } else {
      const s32 rel_data_ofs =
          rsl::load<s32>(data, *tex_desc->anim_handles_offsets[i]);
      const s32 abs_data_ofs =
          *tex_desc->anim_handles_offsets[i] + rel_data_ofs;
      KeyFrameCollection keys;
      if (!ReadKeyFrameCollection(keys, data.subspan(abs_data_ofs)))
        return std::nullopt;
      tex_out.animations[i] = Animation(AnimatedAnimation{.keys = keys});
    }
  }

  return tex_out;
}
#pragma endregion

#if defined(BUILD_DEBUG) && defined(SRT_DEBUG)
static std::vector<u8> sMatchData;
#endif

// TODO: Need writing
// TODO: Need size calc
bool ReadSrtFile(SrtAnimationArchive& arc, std::span<const u8> data) {
#if defined(BUILD_DEBUG) && defined(SRT_DEBUG)
  if (sMatchData.empty())
    sMatchData = {data.data(), data.data() + data.size()};
#endif

  const auto header = ReadSrtHeader(data);
  if (!header.has_value()) {
    return false;
  }

  arc.name = header->name;
  arc.source = header->source;

  arc.frame_duration = header->control.frame_duration;
  arc.xform_model = header->control.xform_model;
  arc.anim_wrap_mode = header->control.wrap_mode;

  arc.mat_animations.resize(header->control.material_count);

  const auto mat_dict = ReadDictionary(data, header->ofs_srt_dict);

  if (!mat_dict.has_value()) {
    return false;
  }

  if (mat_dict->size() != header->control.material_count) {
    return false;
  }

  int i = 0;
  for (auto node : *mat_dict) {
    if (i >= arc.mat_animations.size())
      break;
    auto& mat = arc.mat_animations[i];
    ++i;
    mat.material_name = node.name;

    auto mat_data = ReadSrtMatData(data, node.abs_data_ofs);

    if (!mat_data.has_value()) {
      return false;
    }

    if (mat_data->material_name != node.name) {
      return false;
    }

    for (int i = 0; i < 8; ++i) {
      if (!mat_data->abs_tex_ofs[i].has_value())
        continue;

      const auto texsrt_offset = *mat_data->abs_tex_ofs[i];
      auto tex_anim = ReadTexSrtAnimation(data, texsrt_offset);
      // TODO: Skip bad anims?
      if (!tex_anim.has_value()) {
        return false;
      }

      mat.texture_srt[i] = tex_anim;
    }

    for (int i = 0; i < 3; ++i) {
      if (!mat_data->abs_ind_ofs[i].has_value())
        continue;

      const auto indsrt_offset = *mat_data->abs_ind_ofs[i];
      auto tex_anim = ReadTexSrtAnimation(data, indsrt_offset);
      // TODO: Skip bad anims?
      if (!tex_anim.has_value()) {
        return false;
      }

      mat.indirect_srt[i] = tex_anim;
    }
  }

  return true;
}

void WriteSrtFile(oishii::Writer& writer, const SrtAnimationArchive& arc,
                  NameTable& names, u32 brres_pos) {
#if defined(BUILD_DEBUG) && defined(SRT_DEBUG)
  std::vector<u8> match_data(writer.tell() + sMatchData.size());
  memcpy(match_data.data() + writer.tell(), sMatchData.data(),
         sMatchData.size());
  writer.attachDataForMatchingOutput(match_data);
#endif

  const u32 archive_start_pos = writer.tell();

  // 1. SRT Header
  //
  //    A) Common Header
  //       i. Stream chunk header
  writer.write<u32>('SRT0'); // Fourcc
  const u32 size_deferred = writer.tell();
  writer.write<u32>(0, false); // Size
  //       ii. Sub-node header
  writer.write<u32>(5);                             // Revision
  writer.write<s32>(brres_pos - archive_start_pos); // Parent dictionary
  //    B) SRT-specific Header
  //       i. Root settings
  writer.write<u32>(SrtHeaderBinSize); // Offset to (2) SRT dictionary
  writer.write<u32>(0);                // TODO: User data
  writeNameForward(names, writer, archive_start_pos, arc.name);
  writeNameForward(names, writer, archive_start_pos, arc.source);
  //       ii. SRT Control
  writer.write<u16>(arc.frame_duration);
  assert(arc.mat_animations.size() < 0xFFFF);
  writer.write<u16>(arc.mat_animations.size());
  writer.write<u32>(static_cast<u32>(arc.xform_model));
  writer.write<u32>(static_cast<u32>(arc.anim_wrap_mode));

  // 2. Root Dictionary
  //
  const u32 dict_deferred = writer.tell();
  BetterDictionary dict;
  writer.skip(CalcDictionarySize(arc.mat_animations.size()));

  // 3. Material animation array
  //
  const u32 mat_anim_start = writer.tell();
  const u32 mat_anim_size = [&] {
    u32 val = 0;
    for (auto& m : arc.mat_animations) {
      val += 12;
      for (int i = 0; i < 8; ++i)
        if (m.texture_srt[i].has_value())
          val += 4;
      for (int i = 0; i < 3; ++i)
        if (m.indirect_srt[i].has_value())
          val += 4;
    }
    return val;
  }();
  const u32 mat_anim_end = mat_anim_start + mat_anim_size;

  u32 mat_value_cursor = mat_anim_end;
  for (auto& anim : arc.mat_animations) {
    dict.nodes.push_back(
        BetterNode{.name = anim.material_name, .stream_pos = writer.tell()});
    //    A) Header
    //
    const u32 anim_start = writer.tell();
    SrtMatDataHeader header{.material_name = anim.material_name,
                            .enabled_texsrts = 0,
                            .enabled_indmtxs = 0};
    for (int i = 0; i < 8; ++i)
      header.enabled_texsrts |= (anim.texture_srt[i].has_value() << i);
    for (int i = 0; i < 3; ++i)
      header.enabled_indmtxs |= (anim.indirect_srt[i].has_value() << i);
    writeNameForward(names, writer, anim_start,
                     std::string(header.material_name));
    writer.write<u32>(header.enabled_texsrts);
    writer.write<u32>(header.enabled_indmtxs);

    //    B) Body
    //
    for (auto& val : anim.texture_srt) {
      if (!val.has_value())
        continue;

      writer.write<u32>(mat_value_cursor - anim_start);
      mat_value_cursor += 4; // flags
      for (int i = 0; i < static_cast<int>(SrtAttribute::_Max); ++i) {
        if (!IsSrtAttributeIncluded(val->flags, static_cast<SrtAttribute>(i)))
          continue;

        if (val->animations[i].getType() == StorageFormat::Fixed) {
          mat_value_cursor += 4;
        } else {
          mat_value_cursor += 4;
        }
      }
    }
  }

  writer.seekSet(mat_anim_end);
  u32 keyframe_cursor = mat_value_cursor;

  SimpleMap<KeyFrameCollection, u32>
      collection_array; // Maps a KeyFrameCollection to a stream pos

  // 3. KeyFrameCollection array
  //
  for (auto& anim : arc.mat_animations) {
    for (auto& val : anim.texture_srt) {
      if (!val.has_value())
        continue;

      // auto kf_pos = writer.tell();
      auto header = ConvertFromFlags(val->flags);
      for (int i = 0; i < static_cast<int>(SrtAttribute::_Max); ++i) {
        const bool not_included =
            !IsSrtAttributeIncluded(val->flags, static_cast<SrtAttribute>(i));
        const bool fixed_quant =
            val->animations[i].getType() == StorageFormat::Fixed;

        if (not_included || fixed_quant)
          header.flags |= (0x20 << i);
      }
      writer.write<u32>(header.flags);

      for (int i = 0; i < static_cast<int>(SrtAttribute::_Max); ++i) {
        if (!IsSrtAttributeIncluded(val->flags, static_cast<SrtAttribute>(i)))
          continue;

        if (val->animations[i].getType() == StorageFormat::Fixed) {
          writer.write<f32>(val->animations[i].value);
        } else {
          const auto& keys = val->animations[i].keys;

          if (collection_array.contains(keys)) {
            writer.write<s32>(collection_array[keys] - writer.tell());
          } else {
            writer.write<s32>(keyframe_cursor - writer.tell());
            collection_array.emplace(keys, keyframe_cursor);

            auto back = writer.tell();
            writer.seekSet(keyframe_cursor);

            const auto bin_size = CalcKeyFrameCollectionSize(keys.data.size());
            writer.reserveNext(bin_size);
            WriteKeyFrameCollection({writer.getDataBlockStart() + writer.tell(),
                                     writer.getBufSize() - writer.tell()},
                                    keys);
            writer.skip(bin_size);
            keyframe_cursor += bin_size;

            writer.seekSet(back);
          }
        }
      }
    }
  }
  writer.seekSet(keyframe_cursor);

  const u32 archive_end_pos = writer.tell();

  writer.seekSet(size_deferred);
  writer.write<u32>(archive_end_pos - archive_start_pos);

  writer.seekSet(dict_deferred);
  WriteDictionary(dict, writer, names);

  writer.seekSet(archive_end_pos);
}

} // namespace librii::g3d
