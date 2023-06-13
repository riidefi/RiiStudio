#pragma once

#include <core/common.h>
#include <glm/vec3.hpp>
#include <oishii/forward_declarations.hxx>

namespace librii::j3d {

struct Key {
  float time{};
  float value{};
  float tangentIn{};
  float tangentOut{};
  bool operator==(const Key&) const = default;
};

struct AnimComponent {
  std::vector<Key> scale;
  std::vector<Key> rotation;
  std::vector<Key> translation;
};

struct MaterialAnim {
  glm::vec3 center{};
  std::string name;
  s32 tex_matrix_index{};
  std::vector<Key> scales_x;
  std::vector<Key> scales_y;
  std::vector<Key> scales_z;
  std::vector<Key> rotations_x;
  std::vector<Key> rotations_y;
  std::vector<Key> rotations_z;
  std::vector<Key> translations_x;
  std::vector<Key> translations_y;
  std::vector<Key> translations_z;
  bool operator==(const MaterialAnim&) const = default;
};

enum class LoopType {
  PlayOnce,
  Loop,
  MirrorOnce,
  MirrorLoop,
};

struct BTK {
  std::vector<MaterialAnim> animation_data;
  std::vector<s16> remap_table;
  LoopType loop_mode;

  bool operator==(const BTK&) const = default;

  std::string debugDump();

  Result<void> loadFromFile(std::string_view fileName);
  Result<void> loadFromMemory(std::span<const u8> buf,
                              std::string_view filename);
  Result<void> loadFromStream(oishii::BinaryReader& reader);

  Result<void> load_tag_data_from_file(oishii::BinaryReader& reader,
                                       s32 tag_count);
};

} // namespace librii::j3d
