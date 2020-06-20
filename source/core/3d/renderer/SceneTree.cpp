#include "SceneTree.hpp"

namespace riistudio::lib3d {

void SceneTree::gatherBoneRecursive(u64 boneId, const kpi::FolderData& bones,
                                    const kpi::FolderData& mats,
                                    const kpi::FolderData& polys) {
  const auto& pBone = bones.at<lib3d::Bone>(boneId);

  const u64 nDisplay = pBone.getNumDisplays();
  for (u64 i = 0; i < nDisplay; ++i) {
    const auto display = pBone.getDisplay(i);
    const auto& mat = mats.at<lib3d::Material>(display.matId);

    const auto shader_sources = mat.generateShaders();
    ShaderProgram shader(shader_sources.first, shader_sources.second);
    Node node{mats.at<lib3d::Material>(display.matId),
                    polys.at<lib3d::Polygon>(display.polyId), pBone,
                    display.prio, std::move(shader)};

    auto& nodebuf = node.isTranslucent() ? translucent : opaque;

    nodebuf.push_back(std::make_unique<Node>(std::move(node)));
    nodebuf.back()->mat.observers.push_back(nodebuf.back().get());
  }

  for (u64 i = 0; i < pBone.getNumChildren(); ++i)
    gatherBoneRecursive(pBone.getChild(i), bones, mats, polys);
}

void SceneTree::gather(const kpi::IDocumentNode& root) {
  const auto* pMats = root.getFolder<lib3d::Material>();
  if (pMats == nullptr)
    return;
  const auto* pPolys = root.getFolder<lib3d::Polygon>();
  if (pPolys == nullptr)
    return;

  // We cannot proceed without bones, as we attach all render instructions to
  // them.
  const auto pBones = root.getFolder<lib3d::Bone>();
  if (pBones == nullptr)
    return;

  // Assumes root at zero
  if (pBones->size() == 0)
    return;
  gatherBoneRecursive(0, *pBones, *pMats, *pPolys);
}

} // namespace riistudio::lib3d
