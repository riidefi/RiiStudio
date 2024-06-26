#pragma once

#include <array>
#include <core/common.h>
#include <glm/vec3.hpp>
#include <librii/gx.h>

namespace librii::j3d {

// FIXME: This will exist in our scene and be referenced.
// For now, for 1:1, just placed here..
struct Fog {
  librii::gx::FogType type = librii::gx::FogType::None;
  bool enabled = false;
  u16 center;
  f32 startZ, endZ = 0.0f;
  f32 nearZ, farZ = 0.0f;
  librii::gx::Color color = librii::gx::Color(0xffffffff);
  std::array<u16, 10> rangeAdjTable;

  bool operator==(const Fog& rhs) const noexcept = default;
};

struct NBTScale {
  bool enable = false;
  glm::vec3 scale = {0.0f, 0.0f, 0.0f};

  bool operator==(const NBTScale& rhs) const noexcept = default;
};

struct MaterialData : public librii::gx::GCMaterialData {
  using J3DSamplerData = SamplerData;

  u8 flag = 1;

  // odd data
  librii::j3d::Fog fogInfo{};
  rsl::array_vector<librii::gx::Color, 8> lightColors{};
  librii::j3d::NBTScale nbtScale{};
  // unused data
  // Note: postTexGens are inferred (only enabled counts)
  rsl::array_vector<TexMatrix, 20> postTexMatrices{};
  std::array<u8, 24> stackTrash{}; //!< We have to remember this for 1:1

  bool operator==(const MaterialData& rhs) const = default;
};

} // namespace librii::j3d
