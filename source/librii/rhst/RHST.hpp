#pragma once

#include <core/common.h>           // s8
#include <core/util/timestamp.hpp> // GIT_TAG
#include <glm/vec2.hpp>            // glm::vec3
#include <glm/vec3.hpp>            // glm::vec3
#include <glm/vec4.hpp>            // glm::vec4
#include <optional>
#include <rsl/ArrayVector.hpp> // rsl::array_vector
#include <span>
#include <string>
#include <vector>

namespace librii::rhst {

// Rii hierarchical scene tree

enum class WrapMode { Repeat, Mirror, Clamp };

enum class AlphaMode { Opaque, Clip, Translucent };

struct Material {
  std::string name = "Untitled Material";

  std::string texture_name = "";
  WrapMode wrap_u = WrapMode::Repeat;
  WrapMode wrap_v = WrapMode::Repeat;

  bool show_front = true;
  bool show_back = false;

  AlphaMode alpha_mode = AlphaMode::Opaque;

  s32 lightset_index = -1;
  s32 fog_index = -1;

  std::string preset_path_mdl0mat;

  bool min_filter = true;
  bool mag_filter = true;
  bool enable_mip = true;
  bool mip_filter = true;
  float lod_bias = -1.0f;
};

struct DrawCall {
  s32 mat_index = -1;
  s32 poly_index = -1;
  s32 prio = -1;
};

struct Bone {
  std::string name = "Untitled Bone";
  s32 parent = -1;
  s32 child = -1;
  glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
  glm::vec3 rotate = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 translate = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 min = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 max = glm::vec3(0.0f, 0.0f, 0.0f);

  std::vector<DrawCall> draw_calls;
};

struct Vertex {
  glm::vec3 position{0.0f, 0.0f, 0.0f};
  glm::vec3 normal{0.0f, 0.0f, 0.0f};

  rsl::array_vector<glm::vec2, 8> uvs;
  rsl::array_vector<glm::vec4, 2> colors;
};

enum class Topology { Triangles, TriangleStrip, TriangleFan };

struct Primitive {
  Topology topology = Topology::Triangles;

  std::vector<Vertex> vertices;
};

struct MatrixPrimitive {
  std::array<s32, 10> draw_matrices{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  std::vector<Primitive> primitives;
};

struct Mesh {
  std::string name = "Untitled Mesh";
  s32 current_matrix = 0;
  u32 vertex_descriptor = 0;

  std::vector<MatrixPrimitive> matrix_primitives;
};

struct MetaData {
  std::string format = "JMDL";

  std::string exporter = "RiiStudio";
  std::string exporter_version = GIT_TAG;
};

struct Weight {
  s32 bone_index;
  s32 influence;
};
struct WeightMatrix {
  std::vector<Weight> weights; // all the influences
};

struct SceneTree {
  MetaData meta_data;

  std::vector<Bone> bones;
  std::vector<WeightMatrix> weights;
  std::vector<Mesh> meshes;
  std::vector<Material> materials;
};

std::optional<SceneTree> ReadSceneTree(std::span<const u8> file_data,
                                       std::string& error_message);

} // namespace librii::rhst
