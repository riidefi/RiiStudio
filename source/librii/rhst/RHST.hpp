#pragma once

#include <core/common.h>              // s8
#include <core/util/array_vector.hpp> // riistudio::util::array_vector
#include <core/util/timestamp.hpp>    // GIT_TAG
#include <glm/vec2.hpp>               // glm::vec3
#include <glm/vec3.hpp>               // glm::vec3
#include <glm/vec4.hpp>               // glm::vec4
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace librii::rhst {

template <typename T, int N>
using array_vector = riistudio::util::array_vector<T, N>;

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

  s8 lightset_index = -1;
  s8 fog_index = -1;
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

  array_vector<glm::vec2, 8> uvs;
  array_vector<glm::vec4, 2> colors;
};

enum class PrimitiveType { Triangles, TriangleStrip, TriangleFan };

struct Primitive {
  PrimitiveType primitive_type = PrimitiveType::Triangles;

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

struct SceneTree {
  MetaData meta_data;

  std::vector<Bone> bones;
  std::vector<Mesh> meshes;
  std::vector<Material> materials;
};

std::optional<SceneTree> ReadSceneTree(std::span<const u8> file_data,
                                       std::string& error_message);

} // namespace librii::rhst
