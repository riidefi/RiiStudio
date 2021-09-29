#pragma once

#include <core/common.h>
#include <glm/vec3.hpp>
#include <rsl/SimpleReader.hpp>
#include <span>
#include <string>
#include <vector>

namespace librii::kcol {

// Binary structure
struct Vector3f {
  rsl::bf32 x;
  rsl::bf32 y;
  rsl::bf32 z;
};

// Binary structure
struct KCollisionV1Header {
  rsl::bu32 pos_data_offset;
  rsl::bu32 nrm_data_offset;
  rsl::bu32 prism_data_offset;
  rsl::bu32 block_data_offset;
  rsl::bf32 prism_thickness;
  Vector3f area_min_pos;
  rsl::bu32 area_x_width_mask;
  rsl::bu32 area_y_width_mask;
  rsl::bu32 area_z_width_mask;
  rsl::bs32 block_width_shift;
  rsl::bs32 area_x_blocks_shift;
  rsl::bs32 area_xy_blocks_shift;
  rsl::bf32 sphere_radius;
};

// Binary structure
struct KCollisionPrismData {
  rsl::bf32 height{0.0f};
  rsl::bu16 pos_i{0};
  rsl::bu16 fnrm_i{0};
  rsl::bu16 enrm1_i{0};
  rsl::bu16 enrm2_i{0};
  rsl::bu16 enrm3_i{0};
  rsl::bu16 attribute{0};
};

struct KCollisionData {
  std::vector<glm::vec3> pos_data;
  std::vector<glm::vec3> nrm_data;
  std::vector<KCollisionPrismData> prism_data;
  std::vector<u8> block_data; // TODO: Binary blob, for now
  float prism_thickness = 300.0f;
  glm::vec3 area_min_pos;

  u32 area_x_width_mask = 0;
  u32 area_y_width_mask = 0;
  u32 area_z_width_mask = 0;

  s32 block_width_shift = 0;
  s32 area_x_blocks_shift = 0;
  s32 area_xy_blocks_shift = 0;

  f32 sphere_radius = 250.0f;
};

std::string ReadKCollisionData(KCollisionData& data, std::span<const u8> bytes,
                               u32 file_size);

} // namespace librii::kcol
