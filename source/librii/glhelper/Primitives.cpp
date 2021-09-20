#include "Primitives.hpp"

#include <core/3d/gl.hpp>
#include <librii/gl/Compiler.hpp>
#include <librii/glhelper/ShaderProgram.hpp>
#include <librii/glhelper/VBOBuilder.hpp>

namespace librii::glhelper {

static const float g_vertex_buffer_data[] = {
    -1.0f, -1.0f, -1.0f,                      // triangle 1 : begin
    -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  // triangle 1 : end
    1.0f,  1.0f,  -1.0f,                      // triangle 2 : begin
    -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, // triangle 2 : end
    1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,
    1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, 1.0f,  -1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f,
    -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f,
    -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  1.0f,
    1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,
    1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,
    1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  -1.0f, 1.0f};

const char* gCubeShader = R"(
#version 400

layout(location = 0) in vec3 position;

out vec3 fragmentColor;

layout(std140) uniform ub_SceneParams {
    mat4x4 u_Projection;
    vec4 u_Misc0;
};

void main() {	
  gl_Position = u_Projection * vec4(position, 1);
  fragmentColor = vec3(0.5, 0.5, 1.0);
}
)";
const char* gCubeShaderFrag = R"(
#version 330 core

in vec3 fragmentColor;

out vec3 color;

void main() {
  color = fragmentColor;
}
)";

struct CubeDL {
  librii::glhelper::VBOBuilder mVbo;
  u32 mNumVerts;

  librii::glhelper::ShaderProgram mShader;

  u32 getNumVerts() const { return mNumVerts; }
  u32 getGlId() { return mVbo.getGlId(); }

  u32 getShaderGlId() { return mShader.getId(); }

  CubeDL() : mShader(gCubeShader, gCubeShaderFrag) {
    mVbo.mPropogating[0].descriptor =
        librii::glhelper::VAOEntry{.binding_point = 0,
                                   .name = "position",
                                   .format = GL_FLOAT,
                                   .size = sizeof(glm::vec3)};
    mNumVerts = sizeof(g_vertex_buffer_data) / (sizeof(glm::vec3));
    for (int i = 0; i < mNumVerts; ++i) {
      mVbo.mIndices.push_back(static_cast<u32>(mVbo.mIndices.size()));
      mVbo.pushData(/*binding_point=*/0,
                    (glm::vec3&)g_vertex_buffer_data[i * 3]);
    }
    mVbo.build();
  }
};

std::unique_ptr<CubeDL> gCubeDl;

void InitCube() {
  // We cant init statically, have to wait for Gl to init
  if (!gCubeDl)
    gCubeDl = std::make_unique<CubeDL>();
}

u32 GetCubeVAO() {
  InitCube();
  return gCubeDl->getGlId();
}

u32 GetCubeShader() {
  InitCube();
  return gCubeDl->getShaderGlId();
}

u32 GetCubeNumVerts() {
  InitCube();
  return gCubeDl->getNumVerts();
}

} // namespace librii::glhelper
