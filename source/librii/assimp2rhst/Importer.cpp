#include "Importer.hpp"
#include "Utility.hpp"
#include <LibBadUIFramework/Plugins.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <librii/math/aabb.hpp>
#include <librii/math/srt3.hpp>

namespace librii::assimp2rhst {

AssImporter::AssImporter(const lra::Scene* scene) : pScene(scene) {}
void AssImporter::ProcessMeshTrianglesStatic(
    librii::rhst::Mesh& poly_data,
    std::vector<librii::rhst::Vertex>&& vertices) {
  auto& mp = poly_data.matrix_primitives.emplace_back();
  auto& tris = mp.primitives.emplace_back();
  tris.topology = librii::rhst::Topology::Triangles;
  tris.vertices = std::move(vertices);
}

void AssImporter::ProcessMeshTriangles(
    librii::rhst::Mesh& poly_data, const lra::Mesh* pMesh,
    const lra::Node* pNode, std::vector<librii::rhst::Vertex>&& vertices) {
  ProcessMeshTrianglesStatic(poly_data, std::move(vertices));
}

Result<void> AssImporter::ImportMesh(librii::rhst::SceneTree& out_model,
                                     const lra::Mesh* pMesh,
                                     const lra::Node* pNode, s32 nodeIndex,
                                     glm::vec3 tint) {
  EXPECT(pMesh != nullptr);
  rsl::trace("Importing mesh: {}", pMesh->name);

  auto& poly = out_model.meshes.emplace_back();
  poly.name = pMesh->name;

  EXPECT(pMesh->bones.empty() && "Only singlebound meshes supported!");
  librii::rhst::WeightMatrix search{{
      {static_cast<s32>(nodeIndex), 100},
  }};
  int cur_mtx = -1;
  for (s32 i = 0; i < out_model.weights.size(); ++i) {
    if (out_model.weights[i] == search) {
      cur_mtx = i;
    }
  }
  if (cur_mtx == -1) {
    cur_mtx = static_cast<int>(out_model.weights.size());
    out_model.weights.push_back(search);
  }
  poly.current_matrix = cur_mtx;

  auto& vcd = poly.vertex_descriptor;

  auto add_attribute = [&](auto type, bool direct = false) {
    vcd |= (1 << static_cast<int>(type));
  };
  add_attribute(librii::gx::VertexAttribute::Position);
  if (pMesh->HasNormals()) {
    add_attribute(librii::gx::VertexAttribute::Normal);
  }

  for (int j = 0; j < 2; ++j) {
    if (pMesh->HasVertexColors(j)) {
      add_attribute(librii::gx::VertexAttribute::Color0 + j);
    }
  }

  // Force Color0 for materials
  if (!pMesh->HasVertexColors(0)) {
    add_attribute(librii::gx::VertexAttribute::Color0);
  }

  for (int j = 0; j < 8; ++j) {
    // U and UVW channels already filtered out
    if (pMesh->HasTextureCoords(j)) {
      add_attribute(librii::gx::VertexAttribute::TexCoord0 + j);
    }
  }
  rsl::trace(" ::generating vertices");
  std::vector<librii::rhst::Vertex> vertices;

  for (unsigned f = 0; f < pMesh->faces.size(); ++f) {
    if (pMesh->faces[f].indices.size() != 3) {
      // Skip non-triangle
      rsl::trace("Skipping non-triangle in mesh {}", pMesh->name);
      // Since we split by prim types, we can skip the rest
      out_model.meshes.resize(out_model.meshes.size() - 1);
      return std::unexpected("Mesh has denegerate triangles or points/lines");
    }
    for (int fv = 0; fv < 3; ++fv) {
      const auto v = pMesh->faces[f].indices[fv];

      librii::rhst::Vertex vtx{};
      vtx.position = pMesh->positions[v];
      if (pMesh->HasNormals()) {
        vtx.normal = pMesh->normals[v];
      }
      // We always have at least one pair
      for (int j = 0; j < 2; ++j) {
        if (pMesh->HasVertexColors(j)) {
          auto clr = pMesh->colors[j][v];
          vtx.colors[j] = {clr.r, clr.g, clr.b, clr.a};
          vtx.colors[j] *= glm::vec4(tint, 1.0f);
        }
      }
      if (!pMesh->HasVertexColors(0)) {
        vtx.colors[0] = glm::vec4(tint, 1.0f);
      }
      for (int j = 0; j < 8; ++j) {
        if (pMesh->HasTextureCoords(j)) {
          vtx.uvs[j] = pMesh->uvs[j][v];
        }
      }
      vertices.push_back(vtx);
    }
  }

  ProcessMeshTriangles(poly, pMesh, pNode, std::move(vertices));
  return {};
}

Result<void> AssImporter::ImportNode(librii::rhst::SceneTree& out_model,
                                     const lra::Node* pNode, s32 nodeIndex,
                                     glm::vec3 tint, int parent) {
  // Create a bone (with name)
  auto& joint = out_model.bones.emplace_back();
  joint.name = pNode->name;
  const glm::mat4 xf = pNode->xform;

  librii::math::SRT3 srt;
  glm::quat rotation;
  glm::vec3 skew;
  glm::vec4 perspective;
  glm::decompose(xf, srt.scale, rotation, srt.translation, skew, perspective);

  srt.rotation = {glm::degrees(glm::eulerAngles(rotation).x),
                  glm::degrees(glm::eulerAngles(rotation).y),
                  glm::degrees(glm::eulerAngles(rotation).z)};
  joint.scale = srt.scale;
  joint.rotate = srt.rotation;
  joint.translate = srt.translation;
  rsl::info("Joint {}: S {}, {}, {} R {}, {}, {} T {}, {}, {}", joint.name,
            joint.scale.x, joint.scale.y, joint.scale.z, joint.rotate.x,
            joint.rotate.y, joint.rotate.z, joint.translate.x,
            joint.translate.y, joint.translate.z);

  joint.parent = pNode->parent;

  librii::math::AABB aabb{{FLT_MAX, FLT_MAX, FLT_MAX},
                          {FLT_MIN, FLT_MIN, FLT_MIN}};

  // Mesh data
  for (unsigned i = 0; i < pNode->meshes.size(); ++i) {
    // Can these be duplicated?
    const auto* pMesh = &pScene->meshes[pNode->meshes[i]];

    auto matId = pMesh->materialIndex;
    auto ok = ImportMesh(out_model, pMesh, pNode, nodeIndex, tint);
    if (!ok) {
      rsl::error("Failed to import mesh {}: {}", pMesh->name, ok.error());
      continue;
    }

    aabb.expandBound(librii::math::AABB{
        .min = pMesh->min,
        .max = pMesh->max,
    });
    s32 meshId = out_model.meshes.size() - 1;
    joint.draw_calls.emplace_back(
        librii::rhst::DrawCall{static_cast<s32>(matId), meshId, 0});
  }

  joint.min = aabb.min;
  joint.max = aabb.max;

  for (size_t child : pNode->children) {
    joint.child.push_back(static_cast<s32>(child));
  }

  return {};
}

Result<librii::rhst::SceneTree> AssImporter::Import(const Settings& settings) {
  if (pScene->nodes.empty()) {
    return std::unexpected("No root node");
  }
  librii::rhst::SceneTree out_model;

  std::vector<std::string> new_mats;

  for (unsigned i = 0; i < pScene->materials.size(); ++i) {
    auto* pMat = &pScene->materials[i];
    std::string name = pMat->name;
    if (name.empty()) {
      name = "Material";
      name += std::to_string(i);
    }
    new_mats.push_back(name);
  }

  for (unsigned i = 0; i < pScene->materials.size(); ++i) {
    auto* pMat = &pScene->materials[i];
    auto& mr = out_model.materials.emplace_back();
    mr.name = new_mats[i];

    mr.texture_name = pMat->texture;
    mr.texture_path_hint = pMat->texture_path_hint;
  }

  // TODO: Handle material limitations for samplers..
  for (auto& mat : out_model.materials) {
    mat.enable_mip = true;
    mat.mip_filter = true;
    mat.min_filter = true;
    mat.mag_filter = true;
  }

  for (s32 i = 0; i < pScene->nodes.size(); ++i) {
    auto ok = ImportNode(out_model, &pScene->nodes[i], i, settings.mModelTint);
    if (!ok) {
      return std::unexpected(
          std::format("Failed to import node {}", ok.error()));
    }
  }

  // Vertex alpha default
  for (auto& bone : out_model.bones) {
    for (auto& draw : bone.draw_calls) {
      EXPECT(draw.mat_index < out_model.materials.size());
      auto& mat = out_model.materials[draw.mat_index];
      EXPECT(draw.poly_index < out_model.meshes.size());
      auto& poly = out_model.meshes[draw.poly_index];
      const bool has_vertex_colors =
          poly.vertex_descriptor &
          (1 << static_cast<u32>(librii::gx::VertexAttribute::Color0));
      if (!has_vertex_colors) {
        continue;
      }
      bool has_vertex_alpha = false;
      for (auto& mprim : poly.matrix_primitives) {
        for (auto& primitive : mprim.primitives) {
          for (auto& vtx : primitive.vertices) {
            if (std::fabs(vtx.colors[0].a - 1.0f) >
                std::numeric_limits<f32>::epsilon()) {
              has_vertex_alpha = true;
              goto mprim_loop_end;
            }
          }
        }
      }
    mprim_loop_end:
      if (has_vertex_alpha) {
        mat.alpha_mode = librii::rhst::AlphaMode::Translucent;
      }
    }
  }

  // Add dummy weight
  librii::rhst::WeightMatrix mtx{.weights = {librii::rhst::Weight{
                                     .bone_index = 0,
                                     .influence = 100,
                                 }}};
  out_model.weights.push_back(mtx);
  return out_model;
}

} // namespace librii::assimp2rhst
