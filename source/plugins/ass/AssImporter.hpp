#pragma once

#include <core/common.h>
#include <core/kpi/Plugins.hpp>
#include <glm/glm.hpp>
#include <map>
#include <plugins/gc/Export/IndexedPolygon.hpp>
#include <plugins/j3d/Scene.hpp>
#include <vector>
#include <vendor/assimp/scene.h>

namespace riistudio::ass {

constexpr librii::gx::VertexAttribute PNM =
    librii::gx::VertexAttribute::PositionNormalMatrixIndex;

struct IdCounter {
  u32 boneId = 0;
  u32 meshId = 0;
  u32 matId = 0;

  std::map<const aiNode*, u32> nodeToBoneIdMap;
  std::map<u32, u32> matIdToMatIdMap;
};
class AssImporter {
public:
  AssImporter(const AssImporter&) = delete; // boneIdCtr is not stable
  AssImporter(AssImporter&&) = delete;

  bool assimpSuccess() const {
    return pScene != nullptr && pScene->mRootNode != nullptr;
  }

  AssImporter(const aiScene* scene, kpi::INode* mdl);

  std::set<std::pair<std::size_t, std::string>>
  PrepareAss(bool mip_gen, int min_dim, int max_mip,
             const std::string& model_path);
  void
  ImportAss(const std::vector<std::pair<std::size_t, std::vector<u8>>>& data,
            bool mip_gen, int min_dim, int max_mip, bool auto_outline,
            glm::vec3 tint);

  void SetTransaction(kpi::IOTransaction& t) { transaction = &t; }

private:
  kpi::IOTransaction* transaction = nullptr;
  IdCounter ctr;
  IdCounter* boneIdCtr = &ctr;

  const aiScene* pScene = nullptr;
  libcube::Scene* out_collection = nullptr;
  libcube::Model* out_model = nullptr;
  aiNode* root = nullptr;
  std::vector<u8> scratch;

  int get_bone_id(const aiNode* pNode);
  // Only call if weighted
  u16 add_weight_matrix_low(const libcube::DrawMatrix& drw);
  u16 add_weight_matrix(int v, const aiMesh* pMesh,
                        libcube::DrawMatrix* pDrwOut = nullptr);

  void
  ProcessMeshTrianglesStatic(const aiNode* singleInfluence,
                             libcube::IndexedPolygon& poly_data,
                             std::vector<librii::gx::IndexedVertex>&& vertices);
  void ProcessMeshTrianglesWeighted(
      libcube::IndexedPolygon& poly_data,
      std::vector<librii::gx::IndexedVertex>&& vertices);

  void ProcessMeshTriangles(libcube::IndexedPolygon& poly_data,
                            const aiMesh* pMesh, const aiNode* pNode,
                            std::vector<librii::gx::IndexedVertex>&& vertices);

  bool ImportMesh(const aiMesh* pMesh, const aiNode* pNode, glm::vec3 tint);
  void ImportNode(const aiNode* pNode, glm::vec3 tint, int parent = -1);
};

} // namespace riistudio::ass
