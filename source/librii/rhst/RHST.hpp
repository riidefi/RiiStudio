#pragma once

#include <core/common.h>           // s8
#include <core/util/timestamp.hpp> // GIT_TAG
#include <glm/vec2.hpp>            // glm::vec3
#include <glm/vec3.hpp>            // glm::vec3
#include <glm/vec4.hpp>            // glm::vec4
#include <rsl/ArrayVector.hpp>     // rsl::array_vector
#include <rsl/Timer.hpp>
#include <vendor/magic_enum/magic_enum.hpp>

#include <coro/generator.hpp>

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
inline std::partial_ordering operator<=>(const glm::vec3& l,
                                         const glm::vec3& r) {
  if (auto cmp = l.x <=> r.x; cmp != 0) {
    return cmp;
  }
  if (auto cmp = l.y <=> r.y; cmp != 0) {
    return cmp;
  }
  return l.z <=> r.z;
}

inline std::partial_ordering operator<=>(const glm::vec2& l,
                                         const glm::vec2& r) {
  if (auto cmp = l.x <=> r.x; cmp != 0) {
    return cmp;
  }
  return l.y <=> r.y;
}

#ifdef __APPLE__
namespace std {
template <class I1, class I2, class Cmp>
constexpr auto lexicographical_compare_three_way(I1 f1, I1 l1, I2 f2, I2 l2,
                                                 Cmp comp)
    -> decltype(comp(*f1, *f2)) {
  using ret_t = decltype(comp(*f1, *f2));
  static_assert(std::disjunction_v<std::is_same<ret_t, std::strong_ordering>,
                                   std::is_same<ret_t, std::weak_ordering>,
                                   std::is_same<ret_t, std::partial_ordering>>,
                "The return type must be a comparison category type.");

  bool exhaust1 = (f1 == l1);
  bool exhaust2 = (f2 == l2);
  for (; !exhaust1 && !exhaust2;
       exhaust1 = (++f1 == l1), exhaust2 = (++f2 == l2))
    if (auto c = comp(*f1, *f2); c != 0)
      return c;

  return !exhaust1   ? std::strong_ordering::greater
         : !exhaust2 ? std::strong_ordering::less
                     : std::strong_ordering::equal;
}
} // namespace std
#endif

inline std::partial_ordering operator<=>(const std::array<glm::vec4, 2>& l,
                                         const std::array<glm::vec4, 2>& r) {
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
inline std::partial_ordering operator<=>(const std::array<glm::vec2, 8>& l,
                                         const std::array<glm::vec2, 8>& r) {
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

enum class BillboardMode {
  None = 0,
  Z_Face = 1,
  Z_Parallel = 2,
  ZRotate_Face = 3,
  ZRotate_Parallel= 4,
  Y_Face = 5,
  Y_Parallel = 6,
};

//! Is eventually compiled to a common material.
struct ProtoMaterial {
  std::string name = "Untitled Material";
  bool can_merge = true;

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
  bool can_merge = true;
  BillboardMode billboard_mode = BillboardMode::None;

  s32 parent = -1;
  std::vector<s32> child;
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

  std::array<glm::vec2, 8> uvs{};
  std::array<glm::vec4, 2> colors{};

  // In HW, this is a row index, so multiply by 3
  // Only relevant for rigged models
  // Index into MatrixPrimitive::draw_matrices
  s8 matrix_index{-1};

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

inline size_t VertexCount(const MatrixPrimitive& mp) {
  size_t score = 0;
  for (auto& p : mp.primitives) {
    score += p.vertices.size();
  }
  return score;
}
inline size_t FaceCount(const MatrixPrimitive& mp) {
  u32 face = 0;
  for (auto& p : mp.primitives) {
    if (p.topology == Topology::Triangles) {
      face += p.vertices.size() / 3;
    } else if (p.topology == Topology::TriangleStrip ||
               p.topology == Topology::TriangleFan) {
      face += p.vertices.size() - 2;
    }
  }
  return face;
}

struct Mesh {
  std::string name = "Untitled Mesh";
  bool can_merge = true;

  s32 current_matrix = 0;
  u32 vertex_descriptor = 0;

  std::vector<MatrixPrimitive> matrix_primitives;
};

inline bool hasPosition(u32 vcd) { return vcd & (1 << 9); }
inline bool hasNormal(u32 vcd) { return vcd & (1 << 10); }
inline bool hasColor(u32 vcd, u32 chan) {
  return chan < 2 && vcd & (1 << (11 + chan));
}
inline bool hasTexCoord(u32 vcd, u32 chan) {
  return chan < 8 && vcd & (1 << (13 + chan));
}

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

  std::string name = "course";
  std::vector<Bone> bones;
  std::vector<WeightMatrix> weights;
  std::vector<Mesh> meshes;
  std::vector<ProtoMaterial> materials;
};

struct MeshOptimizerStats {
  // Number of "facepoints"
  u32 before_indices{};
  u32 after_indices{};

  // This includes degenerates
  u32 before_faces{};
  u32 after_faces{};

  // For this algorithm
  u32 ms_elapsed{};

  std::string comment;
};

// Uses zeux/meshoptimizer
Result<MeshOptimizerStats>
StripifyTrianglesMeshOptimizer(MatrixPrimitive& prim);

// Uses GPSnoopy/TriStripper
Result<MeshOptimizerStats> StripifyTrianglesTriStripper(MatrixPrimitive& prim);

// Uses amorilia/tristrip's port of NVTriStrip w/o cache support (fine, our
// target doesn't have a post-TnL cache)
// - Removed use of boost
// - Replaced throw() with assertions
Result<MeshOptimizerStats>
StripifyTrianglesNvTriStripPort(MatrixPrimitive& prim);

// C++ port of jellees/nns-blender-plugin
Result<MeshOptimizerStats> StripifyTrianglesHaroohie(MatrixPrimitive& prim);

Result<MeshOptimizerStats> StripifyTrianglesDraco(MatrixPrimitive& prim,
                                                  bool allow_degen);

Result<MeshOptimizerStats>
ToFanTriangles(MatrixPrimitive& prim, u32 min_len = 4,
               size_t max_runs = std::numeric_limits<size_t>::max());

enum class Algo {
  NvTriStrip,
  Draco,
  Haroohie,
  TriStripper,
  MeshOptmzr,
  DracoDegen,
  RiiFans,
};
Result<MeshOptimizerStats> StripifyTrianglesAlgo(MatrixPrimitive& prim,
                                                 Algo algo);

// Brute-force every algorithm
Result<Algo> StripifyTriangles(MatrixPrimitive& prim,
                               std::optional<Algo> except = std::nullopt,
                               std::string_view debug_name = "?",
                               bool verbose = true);

Result<SceneTree> ReadSceneTree(std::span<const u8> file_data);

} // namespace librii::rhst
