// Based on
// https://github.com/LordNed/JStudio/blob/b9c4eabb1c7e80a8da7f63f8d5003df704de369c/JStudio/J3D/Animation/BTK.cs

#include "BinaryBTK.hpp"
#include <librii/j3d/io/OutputCtx.hpp>
#include <oishii/reader/binary_reader.hxx>

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

static std::string DumpStruct(auto& s) {
#ifdef __clang__
  std::string result;
  auto printer = [&](auto&&... args) {
    char buf[256];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
    snprintf(buf, sizeof(buf), args...);
#pragma clang diagnostic pop
    result += buf;
  };
  __builtin_dump_struct(&s, printer);
  return result;
#else
  return "UNSUPPORTED COMPILER";
#endif
}

std::string BTK::debugDump() {
  std::string result = "DEBUG DUMP OF BTK\n";

  result += DumpStruct(*this);
  for (auto& x : animation_data) {
    result += "ANIMATION DATA ENTRY\n";
    result += DumpStruct(x);
  }

  return result;
}

Result<void> BTK::loadFromFile(std::string_view fileName) {
  auto reader =
      TRY(oishii::BinaryReader::FromFilePath(fileName, std::endian::big));
  return loadFromStream(reader);
}
Result<void> BTK::loadFromMemory(std::span<const u8> buf,
                                 std::string_view filename) {
  oishii::BinaryReader reader(buf, filename, std::endian::big);
  return loadFromStream(reader);
}
Result<void> BTK::loadFromStream(oishii::BinaryReader& reader) {
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

Result<void> BTK::load_tag_data_from_file(oishii::BinaryReader& reader,
                                          s32 tag_count) {
  for (s32 i = 0; i < tag_count; ++i) {
    auto tag_start = reader.tell();

    auto tag_name = TRY(reader.tryRead<u32>());
    auto tag_size = TRY(reader.tryRead<s32>());

    if (tag_name == 'TTK1') {
      loop_mode = static_cast<LoopType>(TRY(reader.tryRead<uint8_t>()));
      auto angle_multiplier = TRY(reader.tryRead<uint8_t>());
      auto anim_length_in_frames = TRY(reader.tryRead<s16>());
      auto texture_anim_entry_count = TRY(reader.tryRead<s16>()) / 3;
      auto num_scale_float_entries = TRY(reader.tryRead<s16>());
      auto num_rotation_short_entries = TRY(reader.tryRead<s16>());
      auto num_translate_float_entries = TRY(reader.tryRead<s16>());
      auto anim_data_offset = TRY(reader.tryRead<s32>());
      auto remap_table_offset = TRY(reader.tryRead<s32>());
      auto string_table_offset = TRY(reader.tryRead<s32>());
      auto texture_index_table_offset = TRY(reader.tryRead<s32>());
      auto texture_center_table_offset = TRY(reader.tryRead<s32>());
      auto scale_data_offset = TRY(reader.tryRead<s32>());
      auto rotation_data_offset = TRY(reader.tryRead<s32>());
      auto translate_data_offset = TRY(reader.tryRead<s32>());

      auto scale_data = TRY(reader.tryReadBuffer<float>(
          num_scale_float_entries, tag_start + scale_data_offset));
      auto rotation_data = TRY(reader.tryReadBuffer<float>(
          num_rotation_short_entries, tag_start + rotation_data_offset));
      auto translation_data = TRY(reader.tryReadBuffer<float>(
          num_translate_float_entries, tag_start + translate_data_offset));

      reader.seek(tag_start + remap_table_offset);
      remap_table = TRY(reader.tryReadBuffer<s16>(texture_anim_entry_count));

      reader.seek(tag_start + string_table_offset);
      auto string_table = TRY(librii::j3d::readNameTable(reader));

      reader.seek(tag_start + texture_index_table_offset);
      auto tex_mtx_index_table =
          TRY(reader.tryReadBuffer<uint8_t>(texture_anim_entry_count));

      reader.seek(tag_start + texture_center_table_offset);
      std::vector<glm::vec3> texture_centers(texture_anim_entry_count);
      for (auto& x : texture_centers) {
        x.x = TRY(reader.tryRead<f32>());
        x.y = TRY(reader.tryRead<f32>());
        x.z = TRY(reader.tryRead<f32>());
      }

      animation_data.reserve(texture_anim_entry_count);
      auto rot_scale = std::pow(2.0f, static_cast<float>(angle_multiplier)) *
                       (180.0 / 32768.0);

      reader.seek(tag_start + anim_data_offset);
      for (s32 j = 0; j < texture_anim_entry_count; ++j) {
        auto tex_u = TRY(read_anim_component(reader));
        auto tex_v = TRY(read_anim_component(reader));
        auto tex_w = TRY(read_anim_component(reader));

        MaterialAnim anim{texture_centers[j],
                          string_table[j],
                          static_cast<s32>(tex_mtx_index_table[j]),
                          read_comp(scale_data, tex_u.scale),
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
