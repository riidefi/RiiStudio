#pragma once

#include <core/3d/i3dmodel.hpp>
#include <core/3d/renderer/ShaderCache.hpp>

namespace riistudio::lib3d {

struct SceneNode : public lib3d::IObserver {
  lib3d::Material& mat;
  const lib3d::Polygon& poly;
  const lib3d::Bone& bone;
  u8 priority;

  const lib3d::Scene& scn;
  const lib3d::Model& mdl;

  ShaderProgram shader;

  // Only used later
  mutable u32 idx_ofs = 0;
  mutable u32 idx_size = 0;
  mutable u32 mtx_id = 0;

  SceneNode(const lib3d::Material& m, const lib3d::Polygon& p,
            const lib3d::Bone& b, u8 prio, const lib3d::Scene& _scn,
            const lib3d::Model& _mdl, ShaderProgram&& prog)
      : mat((lib3d::Material&)m), poly(p), bone(b), priority(prio), scn(_scn),
        mdl(_mdl), shader(std::move(prog)) {}

  void update(lib3d::Material* _mat) override {
    DebugReport("Recompiling shader for %s..\n", _mat->getName().c_str());
    mat = *_mat;
    const auto shader_sources = mat.generateShaders();
    ShaderProgram new_shader(shader_sources.first, _mat->applyCacheAgain
                                                       ? _mat->cachedPixelShader
                                                       : shader_sources.second);
    if (new_shader.getError()) {
      _mat->isShaderError = true;
      _mat->shaderError = new_shader.getErrorDesc();
      return;
    }
    shader = std::move(new_shader);
    _mat->isShaderError = false;
  }

  bool isTranslucent() const { return mat.isXluPass(); }
};

struct RenderPass {
  std::vector<std::unique_ptr<SceneNode>> nodes;

  void zSort() {
	  // FIXME: Implement
  }
};

struct SceneTree {
  using Node = SceneNode;

  RenderPass opaque;
  RenderPass translucent;

  void gatherBoneRecursive(u64 boneId, const lib3d::Model& root,
                           const lib3d::Scene& scn);

  void gather(const lib3d::Model& root, const lib3d::Scene& scene);
};

} // namespace riistudio::lib3d
