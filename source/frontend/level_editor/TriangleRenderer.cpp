#include "TriangleRenderer.hpp"

#include "KclUtil.hpp"
#include <core/3d/gl.hpp>
#include <glm/gtx/norm.hpp> // glm::distance2
#include <librii/gl/Compiler.hpp>

namespace riistudio::lvl {

const char* gTriShader = R"(
#version 400

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in uint attr_id;

out vec4 fragmentColor;
out float _is_wire;

layout(std140) uniform ub_SceneParams {
    mat4x4 u_Projection;
    vec4 u_Misc0;
};

void main() {	
  gl_Position = u_Projection * vec4(position, 1);
  fragmentColor = color;

  // Wire
  bool is_wire = uint(u_Misc0[2]) == 1;
  if (is_wire) {
    // fragmentColor = vec4(1, 1, 1, 1);
    fragmentColor = vec4(mix(normalize(vec3(color)) * sqrt(3), vec3(1, 1, 1), .5), 1);
  }
  _is_wire = is_wire ? 1.0 : 0.0;


  // Precision hack
  uint low_bits = uint(u_Misc0[0]) & 0xFFFF;
  uint high_bits = uint(u_Misc0[1]) << 16;
  uint attr_mask = low_bits | high_bits;

  fragmentColor.w = u_Misc0[3];

  if ((attr_id & attr_mask) == 0) {
    fragmentColor.w = 0.0;
  }
}
)";
const char* gTriShaderFrag = R"(
#version 330 core

in vec4 fragmentColor;
in float _is_wire;

out vec4 color;

void main() {
  if (fragmentColor.w == 0.0) discard;

  color = fragmentColor;

  // bool is_wire = color == vec4(1, 1, 1, 1);
  bool is_wire = _is_wire != 0.0f;

  if (!gl_FrontFacing && !is_wire)
    color *= vec4(.5, .5, .5, 1.0);

  gl_FragDepth = gl_FragCoord.z * (is_wire ? .99991 : .99999);
}
)";

void PushTriangles(riistudio::lib3d::SceneState& state, glm::mat4 modelMtx,
                   glm::mat4 viewMtx, glm::mat4 projMtx, u32 tri_vao,
                   u32 tri_vao_count, u32 attr_mask, float alpha) {
  static const librii::glhelper::ShaderProgram tri_shader(gTriShader,
                                                          gTriShaderFrag);

  auto& cube = state.getBuffers().translucent.nodes.emplace_back();
  static float poly_ofs = 0.0f;
  // ImGui::InputFloat("Poly_ofs", &poly_ofs);
  static float poly_fact = 0.0f;
  // ImGui::InputFloat("Poly_fact", &poly_fact);
  cube.mega_state = {.cullMode = (u32)-1 /* GL_BACK */,
                     .depthWrite = GL_FALSE,
                     .depthCompare = GL_LEQUAL /* GL_LEQUAL */,
                     .frontFace = GL_CCW,

                     .blendMode = GL_FUNC_ADD,
                     .blendSrcFactor = GL_SRC_ALPHA,
                     .blendDstFactor = GL_ONE_MINUS_SRC_ALPHA,

                     // Prevents z-fighting, always win
                     .poly_offset_factor = poly_fact,
                     .poly_offset_units = poly_ofs};
  cube.shader_id = tri_shader.getId();
  cube.vao_id = tri_vao;
  cube.texture_objects.resize(0);

  cube.glBeginMode = GL_TRIANGLES;
  cube.vertex_count = tri_vao_count;
  cube.glVertexDataType = GL_UNSIGNED_INT;
  cube.indices = 0; // Offset in VAO

  cube.bound = {};

  glm::mat4 mvp = projMtx * viewMtx * modelMtx;
  librii::gl::UniformSceneParams params{
      .projection = mvp,
      .Misc0 = {/* attr_mask_lo */ attr_mask & 0xFFFF,
                /* attr_mask_hi */ attr_mask >> 16,
                /* is_wire */ 0.0f, /* alpha */ alpha}};

  enum {
    // So UBOBuilder requires the same UBO layout for every use of the same
    // binding point. It also requires there to be 4x binding point 0, 4x
    // binding point 1, etc.. except it doesn't break if we have extra on
    // binding point 0. This is a mess
    UB_SCENEPARAMS_FOR_CUBE_ID = 0
  };

  auto& uniforms =
      cube.uniform_data.emplace_back(librii::gfx::SceneNode::UniformData{
          .binding_point = UB_SCENEPARAMS_FOR_CUBE_ID,
          .raw_data = {sizeof(params)}});

  uniforms.raw_data.resize(sizeof(params));
  memcpy(uniforms.raw_data.data(), &params, sizeof(params));

  glUniformBlockBinding(
      cube.shader_id, glGetUniformBlockIndex(cube.shader_id, "ub_SceneParams"),
      UB_SCENEPARAMS_FOR_CUBE_ID);

  int query_min = 1024;
  glGetActiveUniformBlockiv(cube.shader_id, UB_SCENEPARAMS_FOR_CUBE_ID,
                            GL_UNIFORM_BLOCK_DATA_SIZE, &query_min);
  // This conflicts with the previous min?

  cube.uniform_mins.push_back(librii::gfx::SceneNode::UniformMin{
      .binding_point = UB_SCENEPARAMS_FOR_CUBE_ID,
      .min_size = static_cast<u32>(query_min)});

  auto& cube_wire = state.getBuffers().translucent.nodes.emplace_back(cube);

  cube_wire.mega_state.fill = librii::gfx::PolygonMode::Line;

  reinterpret_cast<librii::gl::UniformSceneParams*>(
      cube_wire.uniform_data[0].raw_data.data())
      ->Misc0[2] = 1.0f;
}

void TriangleRenderer::convertToTriangles(
    const librii::kcol::KCollisionData& mCourseKcl) {
  mKclTris.resize(mCourseKcl.prism_data.size());
  for (size_t i = 0; i < mKclTris.size(); ++i) {
    auto& prism = mCourseKcl.prism_data[i];

    mKclTris[i] = {.attr = prism.attribute,
                   .verts = librii::kcol::FromPrism(mCourseKcl, prism)};
  }
}

void TriangleRenderer::buildVertexBuffer() {
  tri_vbo = std::make_unique<librii::glhelper::VBOBuilder>();

  tri_vbo->mPropogating[0].descriptor =
      librii::glhelper::VAOEntry{.binding_point = 0,
                                 .name = "position",
                                 .format = GL_FLOAT,
                                 .size = sizeof(glm::vec3)};
  tri_vbo->mPropogating[1].descriptor =
      librii::glhelper::VAOEntry{.binding_point = 1,
                                 .name = "color",
                                 .format = GL_FLOAT,
                                 .size = sizeof(glm::vec4)};
  tri_vbo->mPropogating[2].descriptor =
      librii::glhelper::VAOEntry{.binding_point = 2,
                                 .name = "attr_id",
                                 .format = GL_UNSIGNED_INT,
                                 .size = sizeof(u32)};

  for (int i = 0; i < 3 * mKclTris.size(); ++i) {
    tri_vbo->mIndices.push_back(static_cast<u32>(tri_vbo->mIndices.size()));
    tri_vbo->pushData(/*binding_point=*/0, mKclTris[i / 3].verts[i % 3]);
    tri_vbo->pushData(
        /*binding_point=*/1,
        glm::vec4(GetKCLColor(mKclTris[i / 3].attr), 1.0f));
    tri_vbo->pushData(/*binding_point=*/2,
                      static_cast<u32>(1 << (mKclTris[i / 3].attr & 31)));
  }
  tri_vbo->build();
}

void TriangleRenderer::init(const librii::kcol::KCollisionData& mCourseKcl) {
  convertToTriangles(mCourseKcl);
  buildVertexBuffer();
}

void TriangleRenderer::sortTriangles(const glm::mat4& viewMtx) {
  struct index_tri {
    u32 points[3];

    const glm::vec3& get_point(librii::glhelper::VBOBuilder& tri_vbo,
                               u32 i) const {
      return reinterpret_cast<const glm::vec3&>(
          tri_vbo.mPropogating[0].data[points[i] * sizeof(glm::vec3)]);
    }

    float get_sphere_dist(librii::glhelper::VBOBuilder& tri_vbo, u32 i,
                          const glm::vec3& eye) const {
      // Don't sqrt
      return glm::distance2(get_point(tri_vbo, i), eye);
    }

    float get_min_sphere_dist(librii::glhelper::VBOBuilder& tri_vbo,
                              const glm::vec3& view) const {
      return std::min({get_sphere_dist(tri_vbo, 0, view),
                       get_sphere_dist(tri_vbo, 1, view),
                       get_sphere_dist(tri_vbo, 2, view)});
    }

    float get_plane_dist(librii::glhelper::VBOBuilder& tri_vbo, u32 i,
                         const glm::mat4& view) const {
      // We assume all points are closer, or all are further away
      return (view * glm::vec4(get_point(tri_vbo, i), 1.0f)).z;
    }

    float get_min_plane_dist(librii::glhelper::VBOBuilder& tri_vbo,
                             const glm::mat4& view) const {
      return std::max({get_plane_dist(tri_vbo, 0, view),
                       get_plane_dist(tri_vbo, 1, view),
                       get_plane_dist(tri_vbo, 2, view)});
    }
  };

  index_tri* begin = reinterpret_cast<index_tri*>(tri_vbo->mIndices.data());
  index_tri* end = begin + tri_vbo->mIndices.size() / 3;

  z_dist.resize(end - begin);

  // Assumes no vertex merging, uses first vertex as a key
  for (int i = 0; i < end - begin; ++i) {
    auto first_vtx = begin[i].points[0];
    z_dist[first_vtx / 3] = begin[i].get_min_plane_dist(*tri_vbo, viewMtx);
  }

  auto comp = [&](const index_tri& lhs, const index_tri& rhs) {
    return z_dist[lhs.points[0] / 3] > z_dist[rhs.points[0] / 3];
  };
  std::sort(begin, end, comp);

  std::reverse(begin, end);

  tri_vbo->uploadIndexBuffer();
}

void TriangleRenderer::draw(riistudio::lib3d::SceneState& state,
                            const glm::mat4& modelMtx, const glm::mat4& viewMtx,
                            const glm::mat4& projMtx, u32 attr_mask,
                            float alpha) {
  PushTriangles(state, modelMtx, viewMtx, projMtx, tri_vbo->getGlId(),
                3 * mKclTris.size(), attr_mask, alpha);
}
} // namespace riistudio::lvl
