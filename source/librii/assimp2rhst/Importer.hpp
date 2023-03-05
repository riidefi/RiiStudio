#pragma once

#include <core/common.h>
#include <glm/glm.hpp>
#include <librii/assimp/LRAssimp.hpp>
#include <librii/assimp2rhst/Assimp.hpp>
#include <librii/rhst/RHST.hpp>

namespace librii::assimp2rhst {


class AssImporter {
public:
  AssImporter(const AssImporter&) = delete; // boneIdCtr is not stable
  AssImporter(AssImporter&&) = delete;

  AssImporter(const lra::Scene* scene);
  [[nodiscard]] Result<librii::rhst::SceneTree>
  Import(const Settings& settings);

private:
  const lra::Scene* pScene = nullptr;
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
