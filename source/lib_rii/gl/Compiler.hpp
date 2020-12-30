#pragma once

#include <lib_rii/gx.h>
#include <optional>
#include <string>
#include <string_view>

namespace librii::gl {

// For communicating with your shader:
struct Light {
  glm::vec4 Position = {0, 0, 0, 0};
  glm::vec4 Direction = {0, 0, -1, 0};
  glm::vec4 DistAtten = {0, 0, 0, 0};
  glm::vec4 CosAtten = {0, 0, 0, 0};
  glm::vec4 Color = {1, 1, 1, 1};

  void setWorldPositionViewMatrix(const glm::mat4& viewMatrix, float x, float y,
                                  float z) {
    Position = glm::vec4{x, y, z, 1.0f} * viewMatrix;
  }

  void setWorldDirectionNormalMatrix(const glm::mat4& normalMatrix, float x,
                                     float y, float z) {
    Direction = glm::normalize(glm::vec4(x, y, z, 0.0f) * normalMatrix);
  }
};

// ROW-MAJOR
struct UniformMaterialParams {
  std::array<glm::vec4, 2> ColorMatRegs;
  std::array<glm::vec4, 2> ColorAmbRegs;
  std::array<glm::vec4, 4> KonstColor;
  std::array<glm::vec4, 4> Color;
  std::array<glm::mat3x4, 10> TexMtx; // 4x3
  // sizex, sizey, 0, bias
  std::array<glm::vec4, 8> TexParams;
  std::array<glm::mat2x4, 3> IndTexMtx; // 2x3
  // Optional: Not optional for now.
  std::array<Light, 8> u_LightParams;
};
static_assert(sizeof(UniformMaterialParams) == 1536, "Bad sized UParam");

static_assert(sizeof(UniformMaterialParams) ==
              896 + sizeof(std::array<Light, 8>));
constexpr u32 ub = sizeof(UniformMaterialParams);

struct UniformSceneParams {
  glm::mat4 projection;
  glm::vec4 Misc0;
};
// ROW_MAJOR
struct PacketParams {
  glm::mat3x4 posMtx[10];
};

inline bool setUniformsFromMaterial(const gx::LowLevelGxMaterial& mat) {
  return false;
}

struct GlShaderPair {
  std::string vertex;
  std::string fragment;
};

std::optional<GlShaderPair> compileShader(const gx::LowLevelGxMaterial& mat,
                                          std::string_view name);

} // namespace librii::gl
