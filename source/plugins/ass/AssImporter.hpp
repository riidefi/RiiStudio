#pragma once

#include <core/common.h>
#include <glm/glm.hpp>
#include <map>
#include <plugins/gc/Export/IndexedPolygon.hpp>
#include <plugins/j3d/Scene.hpp>
#include <vector>
#include <vendor/assimp/scene.h>

namespace riistudio::ass {

constexpr libcube::gx::VertexAttribute PNM =
    libcube::gx::VertexAttribute::PositionNormalMatrixIndex;

struct IdCounter {
  u32 boneId = 0;
  u32 meshId = 0;
  u32 matId = 0;

  std::map<const aiNode*, u32> nodeToBoneIdMap;
  std::map<u32, u32> matIdToMatIdMap;
};
class AssImporter {
public:
  bool assimpSuccess() const {
    return pScene != nullptr && pScene->mRootNode != nullptr;
  }

  AssImporter(const aiScene* scene, kpi::INode* mdl);

  std::set<std::pair<std::size_t, std::string>>
  PrepareAss(bool mip_gen, int min_dim, int max_mip);
  void
  ImportAss(const std::vector<std::pair<std::size_t, std::vector<u8>>>& data,
            bool mip_gen, int min_dim, int max_mip, bool auto_outline);

private:
  IdCounter ctr;
  IdCounter* boneIdCtr = &ctr;

  const aiScene* pScene = nullptr;
  j3d::Collection* out_collection = nullptr;
  j3d::Model* out_model = nullptr;
  aiNode* root;
  std::vector<u8> scratch;

  int get_bone_id(const aiNode* pNode);
  // Only call if weighted
  u16 add_weight_matrix_low(const j3d::DrawMatrix& drw);
  u16 add_weight_matrix(int v, const aiMesh* pMesh,
                        j3d::DrawMatrix* pDrwOut = nullptr);

  void
  ProcessMeshTrianglesStatic(const aiNode* singleInfluence,
                             j3d::Shape& poly_data,
                             std::vector<libcube::IndexedVertex>&& vertices);
  void
  ProcessMeshTrianglesWeighted(j3d::Shape& poly_data,
                               std::vector<libcube::IndexedVertex>&& vertices);

  void ProcessMeshTriangles(j3d::Shape& poly_data, const aiMesh* pMesh,
                            const aiNode* pNode,
                            std::vector<libcube::IndexedVertex>&& vertices);

  void ImportMesh(const aiMesh* pMesh, const aiNode* pNode);
  void ImportNode(const aiNode* pNode, int parent = -1);
};

} // namespace riistudio::ass
