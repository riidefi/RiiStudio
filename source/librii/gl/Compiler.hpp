#pragma once

#include <librii/gx.h>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>

namespace librii::gl {

struct VertexAttributeGenDef {
  gx::VertexAttribute attrib;
  const char* name;

  u32 format;
  u32 size;
};

Result<std::pair<const VertexAttributeGenDef*, std::size_t>>
getVertexAttribGenDef(gx::VertexAttribute vtxAttrib);

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

constexpr Light LightOmni = {.Position = glm::vec4{0, 0, 0, 0},
                             .Direction = glm::vec4{0, -1, 0, 0},
                             .DistAtten = glm::vec4{1, 0, 0, 0},
                             .CosAtten = glm::vec4{1, 0, 0, 0},
                             .Color = glm::vec4{0, 0, 0, 1.0f}};

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

template <typename T> inline glm::vec4 colorConvert(T clr) {
  const auto f32c = (gx::ColorF32)clr;
  return {f32c.r, f32c.g, f32c.b, f32c.a};
}

// Does not set:
// - TexMtx
// - TexParams
inline bool setUniformsFromMaterial(UniformMaterialParams& uniform,
                                    const gx::LowLevelGxMaterial& mat) {
  std::fill(uniform.u_LightParams.begin(), uniform.u_LightParams.end(),
            LightOmni);

  for (int i = 0; i < 2; ++i) {
    // TODO: Broken
    uniform.ColorMatRegs[i] =
        glm::vec4{1.0f}; // colorConvert(data.chanData[i].matColor);
    uniform.ColorAmbRegs[i] =
        glm::vec4{1.0f}; // colorConvert(data.chanData[i].ambColor);
  }
  for (int i = 0; i < 4; ++i) {
    uniform.KonstColor[i] = colorConvert(mat.tevKonstColors[i]);
    uniform.Color[i] = colorConvert(mat.tevColors[i]);
  }
  for (int i = 0; i < mat.mIndMatrices.size(); ++i) {
    // TODO: Do we need to transpose (column-major -> row-major)?
    uniform.IndTexMtx[i] = mat.mIndMatrices[i].compute();
  }
  return false;
}

struct GlShaderPair {
  std::string vertex;
  std::string fragment;
};

std::expected<GlShaderPair, std::string>
compileShader(const gx::LowLevelGxMaterial& mat, std::string_view name,
              bool vis_prim = false);

} // namespace librii::gl
