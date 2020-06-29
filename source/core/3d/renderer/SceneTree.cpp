#include "SceneTree.hpp"

namespace riistudio::lib3d {

void SceneTree::gatherBoneRecursive(u64 boneId, const lib3d::Model& root) {
  auto bones = root.getBones();
  auto polys = root.getMeshes();
  auto mats = root.getMaterials();

  const auto& pBone = bones[boneId];
  const u64 nDisplay = pBone.getNumDisplays();

  for (u64 i = 0; i < nDisplay; ++i) {
    const auto display = pBone.getDisplay(i);
    const auto& mat = mats[display.matId];

    const auto shader_sources = mat.generateShaders();
    ShaderProgram shader(shader_sources.first, shader_sources.second);
    Node node{mats[display.matId], polys[display.polyId], pBone, display.prio,
              std::move(shader)};

    auto& nodebuf = node.isTranslucent() ? translucent : opaque;

    nodebuf.push_back(std::make_unique<Node>(std::move(node)));
    nodebuf.back()->mat.observers.push_back(nodebuf.back().get());
  }

  for (u64 i = 0; i < pBone.getNumChildren(); ++i)
    gatherBoneRecursive(pBone.getChild(i), root);
}

void SceneTree::gather(const lib3d::Model& root) {
  if (root.getMaterials().empty() || root.getMeshes().empty() ||
      root.getBones().empty())
    return;

  // Assumes root at zero
  gatherBoneRecursive(0, root);
}

} // namespace riistudio::lib3d
