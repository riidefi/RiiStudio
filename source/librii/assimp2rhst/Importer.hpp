#pragma once

#include <core/common.h>
#include <glm/glm.hpp>
#include <librii/assimp/LRAssimp.hpp>
#include <librii/assimp2rhst/Assimp.hpp>
#include <librii/rhst/RHST.hpp>

namespace librii::assimp2rhst {

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
    return pScene != nullptr && !pScene->nodes.empty();
  }

  AssImporter(const lra::Scene* scene);
  [[nodiscard]] Result<librii::rhst::SceneTree>
  Import(const Settings& settings);

private:
  IdCounter ctr;

  const lra::Scene* pScene = nullptr;
  const lra::Node* root = nullptr;
  void ProcessMeshTrianglesStatic(librii::rhst::Mesh& poly_data,
                                  std::vector<librii::rhst::Vertex>&& vertices);

  void ProcessMeshTriangles(librii::rhst::Mesh& poly_data,
                            const lra::Mesh* pMesh, const lra::Node* pNode,
                            std::vector<librii::rhst::Vertex>&& vertices);

  [[nodiscard]] Result<void> ImportMesh(librii::rhst::SceneTree& out_model,
                                        const lra::Mesh* pMesh,
                                        const lra::Node* pNode, glm::vec3 tint);
  [[nodiscard]] Result<void> ImportNode(librii::rhst::SceneTree& out_model,
                                        const lra::Node* pNode, glm::vec3 tint,
                                        int parent = -1);
};

} // namespace librii::assimp2rhst
