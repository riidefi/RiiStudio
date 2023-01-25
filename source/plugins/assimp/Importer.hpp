#pragma once

#include <core/common.h>
#include <core/kpi/Plugins.hpp>
#include <glm/glm.hpp>
#include <map>
#include <plugins/gc/Export/IndexedPolygon.hpp>
#include <plugins/j3d/Scene.hpp>
#include <vector>
#include <vendor/assimp/scene.h>

#include <librii/rhst/RHST.hpp>

// TODO
#include <plugins/assimp/Assimp.hpp>

namespace riistudio::assimp {

class IdCounter {
public:
  void setConvertedMaterial(u32 aiIndex, u32 matIndex) {
    matIdToMatIdMap[aiIndex] = matIndex;
  }
  Result<u32> getConvertedMaterial(u32 aiIndex) const {
    auto it = matIdToMatIdMap.find(aiIndex);
    EXPECT(it != matIdToMatIdMap.end());
    return it->second;
  }

private:
  std::map<u32, u32> matIdToMatIdMap;
};
class AssImporter {
public:
  AssImporter(const AssImporter&) = delete; // boneIdCtr is not stable
  AssImporter(AssImporter&&) = delete;

  bool assimpSuccess() const {
    return pScene != nullptr && pScene->mRootNode != nullptr;
  }

  AssImporter(const aiScene* scene);
  [[nodiscard]] Result<librii::rhst::SceneTree>
  Import(const Settings& settings);

  void SetTransaction(kpi::IOTransaction& t) { transaction = &t; }

private:
  kpi::IOTransaction* transaction = nullptr;
  IdCounter ctr;

  const aiScene* pScene = nullptr;
  aiNode* root = nullptr;
  void ProcessMeshTrianglesStatic(librii::rhst::Mesh& poly_data,
                                  std::vector<librii::rhst::Vertex>&& vertices);

  void ProcessMeshTriangles(librii::rhst::Mesh& poly_data, const aiMesh* pMesh,
                            const aiNode* pNode,
                            std::vector<librii::rhst::Vertex>&& vertices);

  [[nodiscard]] Result<void> ImportMesh(librii::rhst::SceneTree& out_model,
                                        const aiMesh* pMesh,
                                        const aiNode* pNode, glm::vec3 tint);
  [[nodiscard]] Result<void> ImportNode(librii::rhst::SceneTree& out_model,
                                        const aiNode* pNode, glm::vec3 tint,
                                        int parent = -1);
};

} // namespace riistudio::assimp
