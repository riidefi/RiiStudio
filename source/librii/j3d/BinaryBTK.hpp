#pragma once

#include <core/common.h>
#include <glm/vec3.hpp>
#include <oishii/forward_declarations.hxx>

namespace librii::j3d {

struct KeyFrame {
  float time{};
  float value{};
  float tangentIn{};
  float tangentOut{};
  bool operator==(const KeyFrame&) const = default;
};

using Track = std::vector<KeyFrame>;

struct BinaryMtxAnim {
  Track scales_x;
  Track scales_y;
  Track scales_z;
  Track rotations_x;
  Track rotations_y;
  Track rotations_z;
  Track translations_x;
  Track translations_y;
  Track translations_z;
  bool operator==(const BinaryMtxAnim&) const = default;
};

enum class LoopType {
  PlayOnce,
  Loop,
  MirrorOnce,
  MirrorLoop,
};

/// Low-level direct mapping of the file format
///
/// NOTE: Rotation values are floats, but hold 16-bit FIDX values!
struct BinaryBTK {
  std::vector<u16> remap_table;
  // Only animation_data goes through the remap_table
  std::vector<BinaryMtxAnim> matrices;
  std::vector<std::string> name_table;
  std::vector<u8> tex_mtx_index_table;
  std::vector<glm::vec3> texture_centers;
  LoopType loop_mode{LoopType::PlayOnce};
  // Acts as a factor of (2 << n) on each value
  u8 angle_bitshift{};

  bool operator==(const BinaryBTK&) const = default;

  std::string debugDump();

  Result<void> loadFromFile(std::string_view fileName);
  Result<void> loadFromMemory(std::span<const u8> buf,
                              std::string_view filename);
  Result<void> loadFromStream(oishii::BinaryReader& reader);

  Result<void> load_tag_data_from_file(oishii::BinaryReader& reader,
                                       s32 tag_count);
};

struct TargetedMtx {
  std::string name;
  u8 tex_mtx_index{};
  glm::vec3 texture_center{};
  Track scales_x;
  Track scales_y;
  Track scales_z;
  Track rotations_x;
  Track rotations_y;
  Track rotations_z;
  Track translations_x;
  Track translations_y;
  Track translations_z;
  bool operator==(const TargetedMtx&) const = default;
};

/// Abstracted strcuture
struct BTK {
  std::vector<TargetedMtx> matrices;
  LoopType loop_mode{LoopType::PlayOnce};

  std::string debugDump();

  bool operator==(const BTK&) const = default;

  static Result<BTK> fromBin(const BinaryBTK& bin);
  static Result<BTK> fromFile(std::string_view path);
  static Result<BTK> fromMemory(std::span<const u8> buf, std::string_view path);
};

} // namespace librii::j3d
