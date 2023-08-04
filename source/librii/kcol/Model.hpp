#pragma once

#include <array>
#include <core/common.h>
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <rsl/SimpleReader.hpp>
#include <span>
#include <string>
#include <vector>

#include <version>
#ifdef __cpp_lib_format
#include <format>
#endif
#include <variant>

#include <core/util/timestamp.hpp>

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

inline std::array<glm::vec3, 3> FromPrism(float height, const glm::vec3& pos,
                                          const glm::vec3& fnrm,
                                          const glm::vec3& enrm1,
                                          const glm::vec3& enrm2,
                                          const glm::vec3& enrm3) {
  // Based on wiki
  auto CrossA = glm::cross(enrm1, fnrm);
  auto CrossB = glm::cross(enrm2, fnrm);
  auto Vertex1 = pos;
  auto Vertex2 = pos + CrossB * (height / glm::dot(CrossB, enrm3));
  auto Vertex3 = pos + CrossA * (height / glm::dot(CrossA, enrm3));

  return {Vertex1, Vertex2, Vertex3};
}

struct WiimmKclVersion {
  // 2.26
  int major_version = 2;
  int minor_version = 26;
};

inline std::string FormatWiimmKclVersion(const WiimmKclVersion& ver) {
#ifdef __cpp_lib_format
  return std::format("WiimmSZS v{}.{}", ver.major_version, ver.minor_version);
#else
  return "WiimmSZS v" + std::to_string(ver.major_version) + "." +
         std::to_string(ver.minor_version);
#endif
}

struct UnknownKclVersion {};

inline std::string FormatUnknownKclVersion(const UnknownKclVersion&) {
  return "<Unknown KCL Encoder>";
}

struct InvalidKclVersion {};

inline std::string FormatInvalidKclVersion(const InvalidKclVersion&) {
  return "<Invalid KCL File>";
}

using KclVersion =
    std::variant<WiimmKclVersion, UnknownKclVersion, InvalidKclVersion>;

KclVersion InspectKclFile(std::span<const u8> kcl_file);
std::string GetKCLVersion(KclVersion metadata);

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

  std::string version = std::string("RiiStudio ") + std::string(GIT_TAG);
};

Result<KCollisionData> ReadKCollisionData(std::span<const u8> bytes,
                                          u32 file_size);

inline std::array<glm::vec3, 3> FromPrism(const KCollisionData& data,
                                          const KCollisionPrismData& prism) {
  return FromPrism(prism.height, data.pos_data[prism.pos_i],
                   data.nrm_data[prism.fnrm_i], data.nrm_data[prism.enrm1_i],
                   data.nrm_data[prism.enrm2_i], data.nrm_data[prism.enrm3_i]);
}

std::string DumpJSON(const KCollisionData& map);
KCollisionData LoadJSON(std::string_view map);

} // namespace librii::kcol
