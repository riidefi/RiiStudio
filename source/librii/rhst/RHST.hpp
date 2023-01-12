#pragma once

#include <core/common.h>           // s8
#include <core/util/timestamp.hpp> // GIT_TAG
#include <glm/vec2.hpp>            // glm::vec3
#include <glm/vec3.hpp>            // glm::vec3
#include <glm/vec4.hpp>            // glm::vec4
#include <rsl/ArrayVector.hpp>     // rsl::array_vector
#include <rsl/Timer.hpp>
#include <vendor/magic_enum/magic_enum.hpp>

inline std::partial_ordering operator<=>(const glm::vec4& l,
                                         const glm::vec4& r) {
  if (auto cmp = l.x <=> r.x; cmp != 0) {
    return cmp;
  }
  if (auto cmp = l.y <=> r.y; cmp != 0) {
    return cmp;
  }
  if (auto cmp = l.z <=> r.z; cmp != 0) {
    return cmp;
  }
  return l.w <=> r.w;
}
inline std::partial_ordering operator<=>(const glm::vec2& l,
                                         const glm::vec2& r) {
  if (auto cmp = l.x <=> r.x; cmp != 0) {
    return cmp;
  }
  return l.y <=> r.y;
}

inline std::partial_ordering operator<=>(const std::vector<glm::vec4>& l,
                                         const std::vector<glm::vec4>& r) {
  return std::lexicographical_compare_three_way(
      l.begin(), l.end(), r.begin(), r.end(), [](auto& l, auto& r) {
        if (auto cmp = l.x <=> r.x; cmp != 0) {
          return cmp;
        }
        if (auto cmp = l.y <=> r.y; cmp != 0) {
          return cmp;
        }
        if (auto cmp = l.z <=> r.z; cmp != 0) {
          return cmp;
        }
        return l.w <=> r.w;
      });
}
inline std::partial_ordering operator<=>(const std::vector<glm::vec2>& l,
                                         const std::vector<glm::vec2>& r) {
  return std::lexicographical_compare_three_way(
      l.begin(), l.end(), r.begin(), r.end(), [](auto& l, auto& r) {
        if (auto cmp = l.x <=> r.x; cmp != 0) {
          return cmp;
        }
        return l.y <=> r.y;
      });
}

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

  template <typename T, size_t N> using array_vector = std::vector<T>;

  std::vector<glm::vec2 /* , 8 */> uvs;
  std::vector<glm::vec4 /*, 2 */> colors;

  auto operator<=>(const Vertex& rhs) const = default;
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

// Uses zeux/meshoptimizer
Result<void> StripifyTrianglesMeshOptimizer(MatrixPrimitive& prim);

// Uses GPSnoopy/TriStripper
Result<void> StripifyTrianglesTriStripper(MatrixPrimitive& prim);

// Uses amorilia/tristrip's port of NVTriStrip w/o cache support (fine, our
// target doesn't have a post-TnL cache)
// - Removed use of boost
// - Replaced throw() with assertions
Result<void> StripifyTrianglesNvTriStripPort(MatrixPrimitive& prim);

// C++ port of jellees/nns-blender-plugin
Result<void> StripifyTrianglesHaroohie(MatrixPrimitive& prim);

Result<void> StripifyTrianglesDraco(MatrixPrimitive& prim, bool allow_degen);

extern u64 totalStrippingMs;

enum class Algo {
  MeshOptimizer,
  TriStripper,
  Haroohie,
  Draco,
  DracoDegen,
  NvTriStrip,
};

inline Result<void> StripifyTrianglesAlgo(MatrixPrimitive& prim, Algo algo) {
  fprintf(stderr, "%s :", magic_enum::enum_name(algo).data());
  rsl::Timer timer;
  switch (algo) {
  case Algo::MeshOptimizer:
    StripifyTrianglesMeshOptimizer(prim);
    break;
  case Algo::TriStripper:
    StripifyTrianglesTriStripper(prim);
    break;
  case Algo::NvTriStrip:
    StripifyTrianglesNvTriStripPort(prim);
    break;
  case Algo::Haroohie:
    StripifyTrianglesHaroohie(prim);
    break;
  case Algo::Draco:
    StripifyTrianglesDraco(prim, false);
    break;
  case Algo::DracoDegen:
    StripifyTrianglesDraco(prim, true);
    break;
  }
  totalStrippingMs += timer.elapsed();
  u32 face = 0;
  for (auto& p : prim.primitives) {
    if (p.topology == Topology::Triangles) {
      face += p.vertices.size() / 3;
    } else if (p.topology == Topology::TriangleStrip) {
      face += p.vertices.size() - 2;
    }
  }
  fprintf(stderr, " -> faces: %u\n", face);
  return {};
}

inline Result<void> StripifyTrianglesD(MatrixPrimitive& prim) {
  rsl::Timer timer;
  StripifyTrianglesMeshOptimizer(prim);
  totalStrippingMs += timer.elapsed();
  return {};
}
inline Result<void> StripifyTriangles(MatrixPrimitive& prim) {
  std::vector<MatrixPrimitive> results;
  for (auto e : magic_enum::enum_values<Algo>()) {
    MatrixPrimitive tmp = prim;
    TRY(StripifyTrianglesAlgo(tmp, e));
    results.push_back(std::move(tmp));
  }
  u32 best_score = std::numeric_limits<u32>::max();
  u32 best_index = 0;
  for (size_t i = 0; i < results.size(); ++i) {
    u32 score = 0;
    for (auto& p : results[i].primitives) {
      score += p.vertices.size();
    }
    if (score < best_score) {
      best_score = score;
      best_index = i;
    }
  }
  fprintf(stderr, "%s\n\n",
          std::format("{} won",
                      magic_enum::enum_name(static_cast<Algo>(best_index)))
              .c_str());
  prim = std::move(results[best_index]);
  return {};
}

std::optional<SceneTree> ReadSceneTree(std::span<const u8> file_data,
                                       std::string& error_message);

} // namespace librii::rhst
