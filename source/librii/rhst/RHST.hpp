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

enum class Mapping {
  UVMap,
  EnvMap,
  lEnvMap,
  sEnvMap,
  Projection,
};

enum class WrapMode {
  Repeat,
  Mirror,
  Clamp,
};

enum class AlphaMode {
  Opaque,
  Clip,
  Translucent,
  HpeStencil,
  HpeTranslucent,
  Custom
};

enum class BillboardMode {
  None = 0,
  Z_Face = 1,
  Z_Parallel = 2,
  ZRotate_Face = 3,
  ZRotate_Parallel = 4,
  Y_Face = 5,
  Y_Parallel = 6,
};

enum class AlphaTest {
  Disabled,
  Stencil,
  Custom,
};

enum class Comparison {
  Never,
  Less,
  Equal,
  LEqual,
  Greater,
  NEqual,
  GEqual,
  Always,
};

enum class BlendModeType {
  None,
  Blend,
  Logic,
  Subtract,
};

enum class BlendModeFactor {
  Zero,
  One,
  Src_c,
  Inv_src_c,
  Src_a,
  Inv_src_a,
  Dst_a,
  Inv_dst_a,
};

enum class AlphaOp {
  And,
  Or,
  Xor,
  Xnor,
};

struct PixelEngine {
  //  Alpha Test
  AlphaTest alpha_test = AlphaTest::Stencil;

  // Alpha Comparison
  Comparison comparison_left = Comparison::Always;
  u8 comparison_ref_left = 0;
  AlphaOp comparison_op = AlphaOp::And;
  Comparison comparison_right = Comparison::Always;
  u8 comparison_ref_right = 0;

  // Draw Pass
  bool xlu = false;

  // Z Buffer
  bool z_early_comparison = true;
  bool z_compare = true;
  Comparison z_comparison = Comparison::LEqual;
  bool z_update = true;

  // Dst Alpha
  bool dst_alpha_enabled = false;
  u8 dst_alpha = 0;

  // Blend Mode
  BlendModeType blend_type = BlendModeType::None;
  BlendModeFactor blend_source = BlendModeFactor::Src_a;
  BlendModeFactor blend_dest = BlendModeFactor::Inv_src_a;
  // Skip logic ops for now
};

struct ProtoSampler {
  std::string texture_name = "";
  WrapMode wrap_u = WrapMode::Repeat;
  WrapMode wrap_v = WrapMode::Repeat;

  Mapping mapping = Mapping::UVMap;
  int uv_map_index = 0;
  int light_index = -1;
  int camera_index = -1;

  bool min_filter = true;
  bool mag_filter = true;
  bool enable_mip = true;
  bool mip_filter = true;
  float lod_bias = -1.0f;

  // Matrix

  glm::vec2 scale{1.0f, 1.0f};
  float rotate = 0.0f;
  glm::vec2 trans{0.0f, 0.0f};
};

enum class Colors {
  Red,
  Green,
  Blue,
  Alpha,
};

enum class TevColorArg {
  cprev,
  aprev,
  c0,
  a0,
  c1,
  a1,
  c2,
  a2,
  texc,
  texa,
  rasc,
  rasa,
  one,
  half,
  konst,
  zero
};

enum class TevAlphaArg {
  aprev,
  a0,
  a1,
  a2,
  texa,
  rasa,
  konst,
  zero,
};

enum class TevBias {
  zero,     //!< As-is
  add_half, //!< Add middle gray
  sub_half  //!< Subtract middle gray
};

enum class TevScale {
  scale_1,
  scale_2,
  scale_4,
  divide_2,
};

enum class TevColorOp {
  add,
  subtract,

  comp_r8_gt = 8,
  comp_r8_eq,
  comp_gr16_gt,
  comp_gr16_eq,
  comp_bgr24_gt,
  comp_bgr24_eq,
  comp_rgb8_gt,
  comp_rgb8_eq,
};

enum class TevAlphaOp {
  add,
  subtract,

  comp_r8_gt = 8,
  comp_r8_eq,
  comp_gr16_gt,
  comp_gr16_eq,
  comp_bgr24_gt,
  comp_bgr24_eq,
  // Different from ColorOp
  comp_a8_gt,
  comp_a8_eq,
};

enum class TevKColorSel {
  const_8_8,
  const_7_8,
  const_6_8,
  const_5_8,
  const_4_8,
  const_3_8,
  const_2_8,
  const_1_8,

  k0 = 12,
  k1,
  k2,
  k3,
  k0_r,
  k1_r,
  k2_r,
  k3_r,
  k0_g,
  k1_g,
  k2_g,
  k3_g,
  k0_b,
  k1_b,
  k2_b,
  k3_b,
  k0_a,
  k1_a,
  k2_a,
  k3_a
};

enum class TevKAlphaSel {
  const_8_8,
  const_7_8,
  const_6_8,
  const_5_8,
  const_4_8,
  const_3_8,
  const_2_8,
  const_1_8,

  // Not valid. For generic code
  // {
  k0 = 12,
  k1,
  k2,
  k3,
  // }

  k0_r = 16,
  k1_r,
  k2_r,
  k3_r,
  k0_g,
  k1_g,
  k2_g,
  k3_g,
  k0_b,
  k1_b,
  k2_b,
  k3_b,
  k0_a,
  k1_a,
  k2_a,
  k3_a
};

enum class ColorSelChan {
  color0a0,
  color1a1,

  ind_alpha = 5,
  normalized_ind_alpha,
  null
};

enum class TevReg {
  prev,
  reg0,
  reg1,
  reg2,

  reg3 = prev
};

struct ProtoTevStage {
  ColorSelChan ras_channel = ColorSelChan::null;
  u8 tex_map = 0;
  u8 ras_swap = 0;
  u8 tex_map_swap = 0;

  struct ColorStage {
    TevKColorSel constant_sel = TevKColorSel::k0;
    TevColorArg a = TevColorArg::zero;
    TevColorArg b = TevColorArg::zero;
    TevColorArg c = TevColorArg::zero;
    TevColorArg d = TevColorArg::cprev;
    TevColorOp formula = TevColorOp::add;
    TevBias bias = TevBias::zero;
    TevScale scale = TevScale::scale_1;
    bool clamp = true;
    TevReg out = TevReg::prev;

    bool operator==(const ColorStage& rhs) const noexcept = default;
  } color_stage;

  struct AlphaStage {
    TevKAlphaSel constant_sel = TevKAlphaSel::k0_a;
    TevAlphaArg a = TevAlphaArg::zero;
    TevAlphaArg b = TevAlphaArg::zero;
    TevAlphaArg c = TevAlphaArg::zero;
    TevAlphaArg d = TevAlphaArg::aprev;
    TevAlphaOp formula = TevAlphaOp::add;
    TevBias bias = TevBias::zero;
    TevScale scale = TevScale::scale_1;
    bool clamp = true;
    TevReg out = TevReg::prev;

    bool operator==(const AlphaStage& rhs) const noexcept = default;
  } alpha_stage;

  // Skipping Indirect For now

  bool operator==(const ProtoTevStage& rhs) const noexcept = default;
};

struct ProtoSwapTableEntry {
  Colors r = Colors::Red;
  Colors g = Colors::Green;
  Colors b = Colors::Blue;
  Colors a = Colors::Alpha;
};

//! Is eventually compiled to a common material.
struct ProtoMaterial {
  std::string name = "Untitled Material";
  bool can_merge = true;

  bool show_front = true;
  bool show_back = false;

  AlphaMode alpha_mode = AlphaMode::Opaque;

  PixelEngine pe;

  s32 lightset_index = -1;
  s32 fog_index = -1;

  std::string preset_path_mdl0mat;

  std::vector<ProtoSampler> samplers;

  // Legacy support
  std::string texture_name = "";
  WrapMode wrap_u = WrapMode::Repeat;
  WrapMode wrap_v = WrapMode::Repeat;
  bool min_filter = true;
  bool mag_filter = true;
  bool enable_mip = true;
  bool mip_filter = true;
  float lod_bias = -1.0f;

  std::array<glm::vec4, 4> tev_konst_colors{};
  std::array<glm::vec4, 4> tev_colors{};

  std::vector<ProtoTevStage> tev_stages;
  std::array<ProtoSwapTableEntry, 4> swap_table = {
      ProtoSwapTableEntry{
          .r = Colors::Red,
          .g = Colors::Green,
          .b = Colors::Blue,
          .a = Colors::Alpha,
      },
      ProtoSwapTableEntry{
          .r = Colors::Red,
          .g = Colors::Red,
          .b = Colors::Red,
          .a = Colors::Alpha,
      },
      ProtoSwapTableEntry{
          .r = Colors::Green,
          .g = Colors::Green,
          .b = Colors::Green,
          .a = Colors::Alpha,
      },
      ProtoSwapTableEntry{
          .r = Colors::Blue,
          .g = Colors::Blue,
          .b = Colors::Blue,
          .a = Colors::Alpha,
      },
  };
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

Result<SceneTree> ReadSceneTree(std::span<const u8> file_data);

} // namespace librii::rhst
