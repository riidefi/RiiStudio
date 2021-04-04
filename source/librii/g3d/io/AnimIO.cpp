#include "librii/g3d/data/AnimData.hpp"
#include "librii/g3d/io/CommonIO.hpp"
#include "librii/g3d/io/DictIO.hpp"
#include <algorithm>
#include <rsl/SimpleReader.hpp>
#include <span>
#include <string_view>

namespace librii::g3d {

static void ReadKeyFrame(KeyFrame& out, std::span<const u8> data) {
  assert(data.size_bytes() >= 12);

  out.frame = rsl::load<f32>(data, 0);
  out.value = rsl::load<f32>(data, 4);
  out.slope = rsl::load<f32>(data, 8);
}

static bool ReadKeyFrameCollection(KeyFrameCollection& out,
                                   std::span<const u8> data) {
  constexpr u32 header_size = 8;
  if (data.size_bytes() < header_size)
    return false;

  const u16 keyframe_count = rsl::load<u16>(data, 0);
  const u32 body_size = keyframe_count * 12;

  out.step = rsl::load<f32>(data, 4);

  const u32 required_size = header_size + body_size;
  if (data.size_bytes() < required_size)
    return false;

  out.data.resize(keyframe_count);

  for (size_t i = 0; i < out.data.size(); ++i) {
    ReadKeyFrame(out.data[i], data.subspan(header_size + i * 12, 12));
  }

  // TODO: Warn if out.step != CalcStep

  return true;
}

static AnimationWrapMode ReadAnimationWrapMode(std::span<const u8> data) {
  assert(data.size_bytes() >= 4);

  return static_cast<AnimationWrapMode>(rsl::load<u32>(data, 0));
}

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

struct SrtTexDataHeader {
  u32 flags; // SrtTexFlagVals
};

constexpr u32 SrtTexDataHeaderBinSize = 4;

static SrtTexDataHeader ReadSrtTexDataHeader(std::span<const u8> data) {
  assert(data.size_bytes() >= SrtTexDataHeaderBinSize);

  return SrtTexDataHeader{.flags = rsl::load<u32>(data, 0)};
}

SrtFlags ConvertToFlags(const SrtTexDataHeader& header) {
  return SrtFlags{
      .Unknown = TestSrtTexFlag(header.flags, SrtTexFlagVals::Unknown),
      .ScaleOne = TestSrtTexFlag(header.flags, SrtTexFlagVals::ScaleOne),
      .RotZero = TestSrtTexFlag(header.flags, SrtTexFlagVals::RotZero),
      .TransZero = TestSrtTexFlag(header.flags, SrtTexFlagVals::TransZero),
      .ScaleUniform =
          TestSrtTexFlag(header.flags, SrtTexFlagVals::ScaleIsotropic)};
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
  std::array<std::optional<s32>, NumberOfSrtAttributes> anim_handles_offsets;
};

static std::optional<SrtTexData> ReadSrtTexData(std::span<const u8> whole_data,
                                                unsigned offset) {
  if (whole_data.subspan(offset).size_bytes() < SrtTexDataHeaderBinSize) {
    return std::nullopt;
  }

  const auto header = ReadSrtTexDataHeader(whole_data.subspan(offset));

  SrtTexData result{.flags = ConvertToFlags(header),
                    .attributes_fixed = ConvertToAttrArray(header)};

  unsigned cursor = offset + SrtTexDataHeaderBinSize;

  for (int i = 0; i < NumberOfSrtAttributes; ++i) {
    const SrtAttribute attrib = static_cast<SrtAttribute>(i);

    if (!IsSrtAttributeIncluded(result.flags, attrib))
      continue;

    result.anim_handles_offsets[i] = cursor;
    cursor += 4;
  }

  return result;
}

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

struct SrtMatDataHeader {
  std::string_view material_name;
  u32 enabled_texsrts;
  u32 enabled_indmtxs;
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
    total += IsTexAnimated(header, i) ? 1 : 0;

  return total;
}

struct SrtMatData {
  std::string_view material_name;

  std::array<std::optional<s32>, 8> abs_tex_ofs;
  std::array<std::optional<s32>, 3> abs_ind_ofs;
};

std::optional<SrtMatData> ReadSrtMatData(std::span<const u8> whole_data,
                                         size_t offset) {
  auto data = whole_data.subspan(offset);

  if (data.size_bytes() < SrtMatDataHeaderBinSize) {
    return std::nullopt;
  }

  const auto header = ReadSrtMatDataHeader(data);
  const u32 required_bytes =
      SrtMatDataHeaderBinSize + CalcNumAnimations(header) * 4;

  if (data.size_bytes() < required_bytes) {
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

static std::optional<SrtMatrixAnimation>
ReadTexSrtAnimation(std::span<const u8> data, unsigned texsrt_offset) {
  auto tex_desc = ReadSrtTexData(data, texsrt_offset);
  if (!tex_desc.has_value()) {
    return std::nullopt;
  }

  SrtMatrixAnimation tex_out;
  tex_out.flags = tex_desc->flags;

  for (int i = 0; i < NumberOfSrtAttributes; ++i) {
    if (!tex_desc->anim_handles_offsets[i].has_value())
      continue;

    if (tex_desc->attributes_fixed[i]) {
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

bool ReadSrtFile(SrtAnimationArchive& arc, std::span<const u8> data) {
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

} // namespace librii::g3d
