#include "Importer.hpp"
#include "Material.hpp"
#include "Utility.hpp"
#include <LibBadUIFramework/Plugins.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <librii/math/aabb.hpp>
#include <librii/math/srt3.hpp>

namespace librii::assimp2rhst {

AssImporter::AssImporter(const aiScene* scene) : pScene(scene) {}
void AssImporter::ProcessMeshTrianglesStatic(
    librii::rhst::Mesh& poly_data,
    std::vector<librii::rhst::Vertex>&& vertices) {
  auto& mp = poly_data.matrix_primitives.emplace_back();
  auto& tris = mp.primitives.emplace_back();
  tris.topology = librii::rhst::Topology::Triangles;
  tris.vertices = std::move(vertices);
}

void AssImporter::ProcessMeshTriangles(
    librii::rhst::Mesh& poly_data, const aiMesh* pMesh, const aiNode* pNode,
    std::vector<librii::rhst::Vertex>&& vertices) {
  ProcessMeshTrianglesStatic(poly_data, std::move(vertices));
}

Result<void> AssImporter::ImportMesh(librii::rhst::SceneTree& out_model,
                                     const aiMesh* pMesh, const aiNode* pNode,
                                     glm::vec3 tint) {
  EXPECT(pMesh != nullptr);
  rsl::trace("Importing mesh: {}", pMesh->mName.C_Str());
  // Ignore points and lines
  if (pMesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE) {
    rsl::trace("-> Not triangles; skipping");
    return std::unexpected("Mesh has denegerate triangles or points/lines");
  }

  auto& poly = out_model.meshes.emplace_back();
  poly.name = pMesh->mName.C_Str();
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
    if (pMesh->HasTextureCoords(j)) {
      add_attribute(librii::gx::VertexAttribute::TexCoord0 + j);

      EXPECT(pMesh->mNumUVComponents[j] == 2);
    }
  }
  rsl::trace(" ::generating vertices");
  std::vector<librii::rhst::Vertex> vertices;

  for (unsigned f = 0; f < pMesh->mNumFaces; ++f) {
    for (int fv = 0; fv < 3; ++fv) {
      const auto v = pMesh->mFaces[f].mIndices[fv];

      librii::rhst::Vertex vtx{};
      vtx.position = getVec(pMesh->mVertices[v]);
      if (pMesh->HasNormals()) {
        vtx.normal = getVec(pMesh->mNormals[v]);
      }
      // We always have at least one pair
      for (int j = 0; j < 2; ++j) {
        if (pMesh->HasVertexColors(j)) {
          auto clr = pMesh->mColors[j][v];
          vtx.colors[j] = {clr.r, clr.g, clr.b, clr.a};
          vtx.colors[j] *= glm::vec4(tint, 1.0f);
        }
      }
      if (!pMesh->HasVertexColors(0)) {
        vtx.colors[0] = glm::vec4(tint, 1.0f);
      }
      for (int j = 0; j < 8; ++j) {
        if (pMesh->HasTextureCoords(j)) {
          vtx.uvs[j] = getVec2(pMesh->mTextureCoords[j][v]);
        }
      }
      vertices.push_back(vtx);
    }
  }

  ProcessMeshTriangles(poly, pMesh, pNode, std::move(vertices));
  return {};
}

Result<void> AssImporter::ImportNode(librii::rhst::SceneTree& out_model,
                                     const aiNode* pNode, glm::vec3 tint,
                                     int parent) {
  // Create a bone (with name)
  const auto joint_id = out_model.bones.size();
  auto& joint = out_model.bones.emplace_back();
  joint.name = pNode->mName.C_Str();
  const glm::mat4 xf = getMat4(pNode->mTransformation);

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

  joint.parent = parent;
  if (parent != -1) {
    out_model.bones[parent].child = out_model.bones.size() - 1;
  }

  librii::math::AABB aabb{{FLT_MAX, FLT_MAX, FLT_MAX},
                          {FLT_MIN, FLT_MIN, FLT_MIN}};

  // Mesh data
  for (unsigned i = 0; i < pNode->mNumMeshes; ++i) {
    // Can these be duplicated?
    const auto* pMesh = pScene->mMeshes[pNode->mMeshes[i]];

    auto matId = TRY(ctr.getConvertedMaterial(pMesh->mMaterialIndex));
    auto ok = ImportMesh(out_model, pMesh, pNode, tint);
    if (!ok) {
      rsl::error("Failed to import mesh {}: {}", pMesh->mName.C_Str(),
                 ok.error());
      continue;
    }
    aabb.expandBound(librii::math::AABB{
        .min = getVec(pMesh->mAABB.mMin),
        .max = getVec(pMesh->mAABB.mMax),
    });
    s32 meshId = out_model.meshes.size() - 1;
    joint.draw_calls.emplace_back(
        librii::rhst::DrawCall{static_cast<s32>(matId), meshId, 0});
  }

  joint.min = aabb.min;
  joint.max = aabb.max;

  for (unsigned i = 0; i < pNode->mNumChildren; ++i) {
    auto ok = ImportNode(out_model, pNode->mChildren[i], tint, joint_id);
    if (!ok) {
      rsl::error("Failed to import node {}: {}",
                 pNode->mChildren[i]->mName.C_Str(), ok.error());
      continue;
    }
  }
  return {};
}

auto ConvertWrapMode(aiTextureMapMode mapmode) {
  switch (mapmode) {
  case aiTextureMapMode_Decal:
  case aiTextureMapMode_Clamp:
    return librii::gx::TextureWrapMode::Clamp;
  case aiTextureMapMode_Wrap:
    return librii::gx::TextureWrapMode::Repeat;
  case aiTextureMapMode_Mirror:
    return librii::gx::TextureWrapMode::Mirror;
  case _aiTextureMapMode_Force32Bit:
  default:
    return librii::gx::TextureWrapMode::Repeat;
  }
}

Result<librii::rhst::SceneTree> AssImporter::Import(const Settings& settings) {
  root = pScene->mRootNode;
  librii::rhst::SceneTree out_model;

  std::vector<std::string> new_mats;

  for (unsigned i = 0; i < pScene->mNumMaterials; ++i) {
    auto* pMat = pScene->mMaterials[i];
    std::string name = pMat->GetName().C_Str();
    if (name.empty()) {
      name = "Material";
      name += std::to_string(i);
    }
    new_mats.push_back(name);
  }

  for (unsigned i = 0; i < pScene->mNumMaterials; ++i) {
    auto* pMat = pScene->mMaterials[i];
    auto& mr = out_model.materials.emplace_back();
    mr.name = new_mats[i];
    ctr.setConvertedMaterial(i, i);

    ImpMaterial impMat;

    for (unsigned t = aiTextureType_DIFFUSE; t <= aiTextureType_UNKNOWN; ++t) {
      for (unsigned j = 0; j < pMat->GetTextureCount((aiTextureType)t); ++j) {
        const auto [path, uvindex, mapmode] = GetTexture(pMat, t, j);

        const librii::gx::TextureWrapMode impWrapMode =
            ConvertWrapMode(mapmode);

        ImpSampler impSamp{.path = getFileShort(path.C_Str()),
                           .uv_channel = uvindex,
                           .wrap = impWrapMode};
        impMat.samplers.push_back(impSamp);
      }
    }

    if (impMat.samplers.size() > 0) {
      mr.texture_name = impMat.samplers[0].path;
    }
  }

  // TODO: Handle material limitations for samplers..
  for (auto& mat : out_model.materials) {
    mat.enable_mip = true;
    mat.mip_filter = true;
    mat.min_filter = true;
    mat.mag_filter = true;
  }

  auto ok = ImportNode(out_model, root, settings.mModelTint);
  if (!ok) {
    return std::unexpected(
        std::format("Failed to import root node {}", ok.error()));
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
