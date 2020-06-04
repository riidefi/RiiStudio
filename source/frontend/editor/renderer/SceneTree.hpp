#pragma once

#include <core/3d/i3dmodel.hpp>
#include <frontend/editor/renderer/ShaderCache.hpp>

namespace riistudio::frontend {

struct SceneTree {
  struct Node : public lib3d::IObserver {
    lib3d::Material& mat;
    const lib3d::Polygon& poly;
    const lib3d::Bone& bone;
    u8 priority;

    ShaderProgram& shader;

    // Only used later
    mutable u32 idx_ofs = 0;
    mutable u32 idx_size = 0;
    mutable u32 mtx_id = 0;

    Node(const lib3d::Material& m, const lib3d::Polygon& p,
         const lib3d::Bone& b, u8 prio, ShaderProgram& prog)
        : mat((lib3d::Material&)m), poly(p), bone(b), priority(prio), shader(prog) {}

    void update(lib3d::Material* _mat) override {
      mat = *_mat;
      const auto shader_sources = mat.generateShaders();
      shader =
          ShaderCache::compile(shader_sources.first, shader_sources.second);
    }

    bool isTranslucent() const { return mat.isXluPass(); }
  };

  std::vector<std::unique_ptr<Node>> opaque, translucent;

  void gatherBoneRecursive(u64 boneId, const kpi::FolderData& bones,
                           const kpi::FolderData& mats,
                           const kpi::FolderData& polys);

  void gather(const kpi::IDocumentNode& root);

  // TODO: Z Sort
};

} // namespace riistudio::frontend
