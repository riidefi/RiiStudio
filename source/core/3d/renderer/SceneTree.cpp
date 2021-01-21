#include "SceneTree.hpp"

namespace riistudio::lib3d {

void SceneTree::gatherBoneRecursive(u64 boneId, const lib3d::Model& root,
                                    const lib3d::Scene& scene) {
  auto bones = root.getBones();
  auto polys = root.getMeshes();
  auto mats = root.getMaterials();

  const auto& pBone = bones[boneId];
  const u64 nDisplay = pBone.getNumDisplays();

  for (u64 i = 0; i < nDisplay; ++i) {
    const auto display = pBone.getDisplay(i);
    const auto& mat = mats[display.matId];

    const auto shader_sources = mat.generateShaders();
    librii::glhelper::ShaderProgram shader(shader_sources.first,
                                           shader_sources.second);
    assert(display.matId < mats.size());
    assert(display.polyId < polys.size());
    Node node{mats[display.matId],
              polys[display.polyId],
              pBone,
              display.prio,
              scene,
              root,
              std::move(shader)};

    auto& nodebuf = node.isTranslucent() ? translucent : opaque;

    nodebuf.nodes.push_back(std::make_unique<Node>(std::move(node)));
    nodebuf.nodes.back()->mat.observers.push_back(nodebuf.nodes.back().get());
  }

  for (u64 i = 0; i < pBone.getNumChildren(); ++i)
    gatherBoneRecursive(pBone.getChild(i), root, scene);
}

} // namespace riistudio::lib3d
