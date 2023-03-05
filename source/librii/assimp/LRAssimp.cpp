#include "LRAssimp.hpp"
#include <core/common.h>
#include <vendor/assimp/scene.h>

namespace librii::lra {

namespace {

SceneAttrs ReadSceneAttrs(const aiScene& scene) {
  SceneAttrs a;
  u32 flags = scene.mFlags;
  if (flags & AI_SCENE_FLAGS_INCOMPLETE) {
    a.Incomplete = true;
  }
  if (flags & AI_SCENE_FLAGS_VALIDATED) {
    a.Validated = true;
  }
  if (flags & AI_SCENE_FLAGS_VALIDATION_WARNING) {
    a.HasValidationIssues = true;
  }
  if (flags & AI_SCENE_FLAGS_NON_VERBOSE_FORMAT) {
    a.HasSharedVertices = true;
  }
  if (flags & AI_SCENE_FLAGS_TERRAIN) {
    // We ignore
  }
  if (flags & AI_SCENE_FLAGS_ALLOW_SHARED) {
    // We ignore
  }
  return a;
}
template <typename T> std::vector<T> ReadVec(const T* it, unsigned int len) {
  if (it == nullptr || len == 0) {
    return {};
  }
  return {it, it + len};
}
Face ReadFace(const aiFace& face) {
  return {.indices = ReadVec(face.mIndices, face.mNumIndices)};
}

Mesh ReadMesh(const aiMesh& mesh) {
  (void)mesh.mPrimitiveTypes;
  Mesh m;
  m.positions = ReadVec(reinterpret_cast<const glm::vec3*>(mesh.mVertices),
                        mesh.mNumVertices);
  m.normals = ReadVec(reinterpret_cast<const glm::vec3*>(mesh.mNormals),
                      mesh.mNumVertices);
  (void)mesh.mTangents;
  (void)mesh.mBitangents;
  static_assert(AI_MAX_NUMBER_OF_COLOR_SETS == m.colors.size());
  for (size_t i = 0; i < m.colors.size(); ++i) {
    m.colors[i] = ReadVec(reinterpret_cast<const glm::vec4*>(mesh.mColors[i]),
                          mesh.mNumVertices);
  }
  static_assert(AI_MAX_NUMBER_OF_COLOR_SETS == m.uvs.size());
  for (size_t i = 0; i < m.uvs.size(); ++i) {
    if (mesh.mNumUVComponents[i] != 2) {
      rsl::warn("Mesh {}: Skipping channel {} as the number of components is "
                "{}, when we only support 2 (UV)",
                mesh.mName.C_Str(), i, mesh.mNumUVComponents[i]);
      continue;
    }
    auto of_vec3 =
        ReadVec(reinterpret_cast<const glm::vec3*>(mesh.mTextureCoords[i]),
                mesh.mNumVertices);
    m.uvs[i].resize(of_vec3.size());
    for (size_t j = 0; j < m.uvs[i].size(); ++j) {
      m.uvs[i][j] = of_vec3[j];
    }
  }
  if (mesh.mFaces && mesh.mNumFaces) {
    m.faces.resize(mesh.mNumFaces);
    for (size_t i = 0; i < m.faces.size(); ++i) {
      m.faces[i] = ReadFace(mesh.mFaces[i]);
    }
  }
  (void)mesh.mNumBones;
  (void)mesh.mBones;
  m.materialIndex = mesh.mMaterialIndex;
  m.name = mesh.mName.C_Str();
  (void)mesh.mNumAnimMeshes;
  (void)mesh.mAnimMeshes;
  (void)mesh.mMethod;
  m.min = std::bit_cast<glm::vec3>(mesh.mAABB.mMin);
  m.max = std::bit_cast<glm::vec3>(mesh.mAABB.mMax);
  return m;
}

std::vector<Mesh> ReadMeshes(const aiScene& scn) {
  if (scn.mMeshes && scn.mNumMeshes) {
    std::vector<Mesh> result(scn.mNumMeshes);
    for (size_t i = 0; i < result.size(); ++i) {
      result[i] = ReadMesh(*scn.mMeshes[i]);
    }
    return result;
  }
  return {};
}

// Old code, taken from Material.hpp
namespace {
struct ImpSampler {
  std::string path;
  u32 uv_channel;
};
struct ImpMaterial {
  std::vector<ImpSampler> samplers;
};
static inline std::string getFileShort(const std::string& path) {
  auto tmp = path.substr(path.rfind("\\") + 1);
  tmp = tmp.substr(0, tmp.rfind("."));
  return tmp;
}
static std::tuple<aiString, unsigned int, aiTextureMapMode>
GetTexture(const aiMaterial* pMat, int t, int j) {
  aiString path;
  // No support for anything non-uv -- already processed away by the time
  // we get it aiTextureMapping mapping;
  unsigned int uvindex = 0;
  // We don't support blend/op.
  // All the data here *could* be translated to TEV,
  // but it isn't practical.
  // ai_real blend;
  // aiTextureOp op;
  // No decal support
  u32 mapmode = aiTextureMapMode_Wrap;

  pMat->GetTexture(static_cast<aiTextureType>(t), j, &path, nullptr, &uvindex,
                   nullptr, nullptr, nullptr //(aiTextureMapMode*)&mapmode
  );
  return {path, uvindex, (aiTextureMapMode)mapmode};
}
ImpMaterial ReadImp(const aiMaterial& mat) {
  ImpMaterial impMat;

  for (unsigned t = aiTextureType_DIFFUSE; t <= aiTextureType_UNKNOWN; ++t) {
    for (unsigned j = 0; j < mat.GetTextureCount((aiTextureType)t); ++j) {
      const auto [path, uvindex, mapmode] = GetTexture(&mat, t, j);

      ImpSampler impSamp{
          .path = getFileShort(path.C_Str()),
          .uv_channel = uvindex,
      };
      impMat.samplers.push_back(impSamp);
    }
  }
  return impMat;
}
static inline glm::mat4 getMat4(const aiMatrix4x4& mtx) {
  glm::mat4 out;

  out[0][0] = mtx.a1;
  out[1][0] = mtx.a2;
  out[2][0] = mtx.a3;
  out[3][0] = mtx.a4;
  out[0][1] = mtx.b1;
  out[1][1] = mtx.b2;
  out[2][1] = mtx.b3;
  out[3][1] = mtx.b4;
  out[0][2] = mtx.c1;
  out[1][2] = mtx.c2;
  out[2][2] = mtx.c3;
  out[3][2] = mtx.c4;
  out[0][3] = mtx.d1;
  out[1][3] = mtx.d2;
  out[2][3] = mtx.d3;
  out[3][3] = mtx.d4;
  return out;
}
} // namespace
Material ReadMaterial(const aiMaterial& mat) {
  auto imp = ReadImp(mat);
  std::string tex = imp.samplers.empty() ? "" : imp.samplers[0].path;
  return {
      .name = const_cast<aiMaterial&>(mat).GetName().C_Str(),
      .texture = tex,
  };
}
std::vector<Material> ReadMaterials(const aiScene& scn) {
  if (scn.mMaterials && scn.mNumMaterials) {
    std::vector<Material> result(scn.mNumMaterials);
    for (size_t i = 0; i < result.size(); ++i) {
      result[i] = ReadMaterial(*scn.mMaterials[i]);
    }
    return result;
  }
  return {};
}

// Does not read children yet
Node ReadNode(const aiNode& node) {
  Node n;
  n.name = node.mName.C_Str();
  n.xform = getMat4(node.mTransformation);
  (void)node.mParent;
  (void)node.mChildren; // Read later
  n.meshes = ReadVec(node.mMeshes, node.mNumMeshes);
  (void)node.mMetaData;
  return n;
}

// Recursive function
void ReadNodesImpl(const aiNode* root, auto&& out_it,
                   std::set<aiNode*>& ids_set) {
  // ids_set:
  // - To detect circular references / duplicates
  // - Sorted to allow aiNode* -> ID lookup
  auto n = ReadNode(*root);
  *out_it++ = n;
  auto children = ReadVec(root->mChildren, root->mNumChildren);
  for (size_t i = 0; i < children.size(); ++i) {
    auto* it = children[i];
    if (it == nullptr) {
      continue;
    }
    if (ids_set.contains(it)) {
      rsl::warn("Node {} (#{}) has already been seen. Skipping to avoid "
                "circular reference",
                it->mName.C_Str(), i);
      continue;
    }
    n.children.push_back(ids_set.size());
    ids_set.emplace(it);
    ReadNodesImpl(it, out_it, ids_set);
  }
}

std::vector<Node> ReadNodes(const aiScene& scn) {
  if (scn.mRootNode == nullptr) {
    return {};
  }
  std::vector<Node> result;
  std::set<aiNode*> ids_set;
  ids_set.emplace(scn.mRootNode);
  ReadNodesImpl(scn.mRootNode, std::back_inserter(result), ids_set);
  return result;
}

} // namespace

Scene ReadScene(const aiScene& scn) {
  return {
      .flags = ReadSceneAttrs(scn),
      .nodes = ReadNodes(scn),
      .meshes = ReadMeshes(scn),
      .materials = ReadMaterials(scn),
      .animations = {},
  };
}

void DropNonTriangularMeshes(Scene& scn) {
  for (size_t i = 0; i < scn.meshes.size(); ++i) {
    bool is_bad = false;
    for (auto& face : scn.meshes[i].faces) {
      if (face.indices.size() != 3) {
        is_bad = true;
        break;
      }
    }
    if (is_bad) {
      rsl::info("Mesh {} contained non-triangles; dropping those",
                scn.meshes[i].name);
      scn.meshes.erase(scn.meshes.begin() + i);
      // Update references
      for (auto& node : scn.nodes) {
        for (size_t j = 0; j < node.meshes.size(); ++j) {
          if (node.meshes[j] == i) {
            node.meshes.erase(node.meshes.begin() + j);
            --j;
          } else if (node.meshes[j] > i) {
            --node.meshes[j];
          }
        }
      }
      --i;
    }
  }
}

void MakeMeshNamesUnique(Scene& scn) {
  std::set<std::string> mesh_names;
  size_t i = 0;
  for (auto& m : scn.meshes) {
    if (mesh_names.contains(m.name)) {
      m.name = std::format("{}#{}", m.name, i);
    }
    mesh_names.emplace(m.name);
    ++i;
  }
}

} // namespace librii::lra
