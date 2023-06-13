// Based on
// https://github.com/LordNed/JStudio/blob/b9c4eabb1c7e80a8da7f63f8d5003df704de369c/JStudio/J3D/Animation/BTK.cs

#include <rsl/Reflection.hpp>

#include "BinaryBTK.hpp"
#include <librii/j3d/io/OutputCtx.hpp>
#include <oishii/reader/binary_reader.hxx>
#include <rsl/DumpStruct.hpp>

namespace librii::j3d {

enum class TangentType {
  TangentIn,
  TangentInOut,
};
struct BinAnimIndex {
  u16 count;
  u16 index;
  TangentType keyTangentType;
};

struct BinAnimComponent {
  BinAnimIndex scale;
  BinAnimIndex rotation;
  BinAnimIndex translation;
};

Result<BinAnimIndex> ReadAnimIndex(oishii::BinaryReader& stream) {
  BinAnimIndex animIndex;
  animIndex.count = TRY(stream.tryRead<u16>());
  animIndex.index = TRY(stream.tryRead<u16>());
  animIndex.keyTangentType =
      static_cast<TangentType>(TRY(stream.tryRead<u16>()));
  return animIndex;
}

Result<BinAnimComponent> read_anim_component(oishii::BinaryReader& reader) {
  BinAnimComponent animComponent;
  animComponent.scale = TRY(ReadAnimIndex(reader));
  animComponent.rotation = TRY(ReadAnimIndex(reader));
  animComponent.translation = TRY(ReadAnimIndex(reader));
  return animComponent;
}

std::vector<Key> read_comp(std::span<const float> src,
                           const BinAnimIndex& index) {
  std::vector<Key> ret;

  if (index.count == 1) {
    ret.emplace_back(Key{
        .time = 0.0f,
        .value = src[index.index],
        .tangentIn = 0.0f,
        .tangentOut = 0.0f,
    });
  } else {
    for (int j = 0; j < index.count; j++) {
      Key key;
      if (index.keyTangentType == TangentType::TangentIn) {
        key = {
            .time = src[index.index + 3 * j],
            .value = src[index.index + 3 * j + 1],
            .tangentIn = src[index.index + 3 * j + 2],
            .tangentOut = src[index.index + 3 * j + 2],
        };
      } else {
        key = {
            .time = src[index.index + 4 * j],
            .value = src[index.index + 4 * j + 1],
            .tangentIn = src[index.index + 4 * j + 2],
            .tangentOut = src[index.index + 4 * j + 3],
        };
      }
      ret.push_back(key);
    }
  }

  return ret;
}
void convert_rotation(std::vector<Key>& rots, float scale) {
  for (auto& key : rots) {
    key.value *= scale;
    key.tangentIn *= scale;
    key.tangentOut *= scale;
  }
}

std::string BinaryBTK::debugDump() {
  std::string result = "DEBUG DUMP OF BTK\n";

  result += rsl::DumpStruct(*this);
  for (auto& x : animation_data) {
    result += "ANIMATION DATA ENTRY\n";
    result += rsl::DumpStruct(x);
  }

  return result;
}

Result<void> BinaryBTK::loadFromFile(std::string_view fileName) {
  auto reader =
      TRY(oishii::BinaryReader::FromFilePath(fileName, std::endian::big));
  return loadFromStream(reader);
}
Result<void> BinaryBTK::loadFromMemory(std::span<const u8> buf,
                                       std::string_view filename) {
  oishii::BinaryReader reader(buf, filename, std::endian::big);
  return loadFromStream(reader);
}
Result<void> BinaryBTK::loadFromStream(oishii::BinaryReader& reader) {
  auto magic = TRY(reader.tryRead<u32>());
  EXPECT(magic == 'J3D1');
  auto anim_type = TRY(reader.tryRead<u32>());
  EXPECT(anim_type == 'btk1');
  auto file_size = TRY(reader.tryRead<s32>());
  auto tag_count = TRY(reader.tryRead<s32>());
  // Skip SVR1
  reader.seekSet(0x20);
  TRY(load_tag_data_from_file(reader, tag_count));

  return {};
}

struct TTK1Header {
  u8 loop_mode{};
  uint8_t angle_multiplier{};
  s16 anim_length_in_frames{};
  // div3
  s16 texture_anim_entry_count{};
  s16 num_scale_float_entries{};
  s16 num_rotation_short_entries{};
  s16 num_translate_float_entries{};
  s32 anim_data_offset{};
  s32 remap_table_offset{};
  s32 string_table_offset{};
  s32 texture_index_table_offset{};
  s32 texture_center_table_offset{};
  s32 scale_data_offset{};
  s32 rotation_data_offset{};
  s32 translate_data_offset{};
};

Result<TTK1Header> ReadTTK1Header(oishii::BinaryReader& reader) {
  rsl::SafeReader safe(reader);
  TTK1Header out;
  rsl::ReadFields(out, safe);
  return out;
}

Result<void> BinaryBTK::load_tag_data_from_file(oishii::BinaryReader& reader,
                                                s32 tag_count) {
  for (s32 i = 0; i < tag_count; ++i) {
    auto tag_start = reader.tell();

    auto tag_name = TRY(reader.tryRead<u32>());
    auto tag_size = TRY(reader.tryRead<s32>());

    if (tag_name == 'TTK1') {
      auto ttk1 = TRY(ReadTTK1Header(reader));
      loop_mode = TRY(rsl::enum_cast<LoopType>(ttk1.loop_mode));

      // J3DMaterialTable::createTexMtxForAnimator shows this to be the size of
      // the remap table, but not the data arrays necessarily
      auto remap_table_count = ttk1.texture_anim_entry_count / 3;

      reader.seek(tag_start + ttk1.remap_table_offset);
      remap_table = TRY(reader.tryReadBuffer<u16>(remap_table_count));

      // (and contiguous)
      bool strictlyIncreasing = true;
      for (size_t i = 0; i < remap_table.size(); ++i) {
        if (remap_table[i] != i) {
          strictlyIncreasing = false;
        }
      }
      if (!strictlyIncreasing) {
        rsl::error("TTK1 data is compressed!");
      }

      reader.seek(tag_start + ttk1.string_table_offset);
      name_table = TRY(librii::j3d::readNameTable(reader));
      EXPECT(name_table.size() == remap_table_count);

      auto scale_data = TRY(reader.tryReadBuffer<float>(
          ttk1.num_scale_float_entries, tag_start + ttk1.scale_data_offset));
      auto rotation_data_short =
          TRY(reader.tryReadBuffer<s16>(ttk1.num_rotation_short_entries,
                                        tag_start + ttk1.rotation_data_offset));
      std::vector<f32> rotation_data;
      for (s16 s : rotation_data_short) {
        rotation_data.push_back(static_cast<f32>(s));
      }
      auto translation_data = TRY(
          reader.tryReadBuffer<float>(ttk1.num_translate_float_entries,
                                      tag_start + ttk1.translate_data_offset));

      reader.seek(tag_start + ttk1.texture_index_table_offset);
      tex_mtx_index_table =
          TRY(reader.tryReadBuffer<uint8_t>(remap_table_count));

      reader.seek(tag_start + ttk1.texture_center_table_offset);
      texture_centers.resize(remap_table_count);
      for (auto& x : texture_centers) {
        x.x = TRY(reader.tryRead<f32>());
        x.y = TRY(reader.tryRead<f32>());
        x.z = TRY(reader.tryRead<f32>());
      }

      animation_data.reserve(remap_table_count);
      auto rot_scale =
          std::pow(2.0f, static_cast<float>(ttk1.angle_multiplier)) *
          (180.0 / 32768.0);

      reader.seek(tag_start + ttk1.anim_data_offset);
	  
      // Having the compressed component array using remap_table_count is
      // actually a failure of my code
      for (s32 j = 0; j < remap_table_count; ++j) {
        auto tex_u = TRY(read_anim_component(reader));
        auto tex_v = TRY(read_anim_component(reader));
        auto tex_w = TRY(read_anim_component(reader));

        MaterialAnim anim{read_comp(scale_data, tex_u.scale),
                          read_comp(scale_data, tex_v.scale),
                          read_comp(scale_data, tex_w.scale),
                          read_comp(rotation_data, tex_u.rotation),
                          read_comp(rotation_data, tex_v.rotation),
                          read_comp(rotation_data, tex_w.rotation),
                          read_comp(translation_data, tex_u.translation),
                          read_comp(translation_data, tex_v.translation),
                          read_comp(translation_data, tex_w.translation)};

        convert_rotation(anim.rotations_x, rot_scale);
        convert_rotation(anim.rotations_y, rot_scale);
        convert_rotation(anim.rotations_z, rot_scale);

        animation_data.push_back(std::move(anim));
      }
    } else {
      // ...
    }

    reader.seek(tag_start + tag_size);
  }

  return {};
}

} // namespace librii::j3d
