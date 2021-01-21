#pragma once

#include <core/3d/i3dmodel.hpp>
#include <librii/glhelper/ShaderCache.hpp>
#include <map>

namespace riistudio::lib3d {

// Not implemented yet
enum class RenderPass {
  Standard,
  ID // For selection
};

struct SceneNode : public lib3d::IObserver {
  virtual ~SceneNode() = default;

  // Draw the node to the screen
  virtual void draw(librii::glhelper::VBOBuilder& vbo_builder,
                    librii::glhelper::DelegatedUBOBuilder& ubo_builder,
                    const std::map<std::string, u32>& tex_id_map) = 0;

  // Expand an AABB with the current bounding box
  //
  // Note: Model-space
  virtual void expandBound(AABB& bound) = 0;

  // Fill-in a UBO
  //
  // Called every frame
  virtual void
  buildUniformBuffer(librii::glhelper::DelegatedUBOBuilder& ubo_builder,
                     const std::map<std::string, u32>& tex_id_map, u32 _mtx_id,
                     const glm::mat4& model_matrix,
                     const glm::mat4& view_matrix,
                     const glm::mat4& proj_matrix) = 0;

  // Fill-in a VBO, remembering the start/end index
  //
  // Called once (or when vertex data is changed)
  virtual void buildVertexBuffer(librii::glhelper::VBOBuilder& vbo_builder) = 0;
};

struct GCSceneNode : public SceneNode {
  GCSceneNode(const lib3d::Material& m, const lib3d::Polygon& p,
              const lib3d::Bone& b, const lib3d::Scene& _scn,
              const lib3d::Model& _mdl, librii::glhelper::ShaderProgram&& prog);

  void draw(librii::glhelper::VBOBuilder& vbo_builder,
            librii::glhelper::DelegatedUBOBuilder& ubo_builder,
            const std::map<std::string, u32>& tex_id_map) final;
  void expandBound(AABB& bound) final;

  void update(lib3d::Material* _mat) final;

  void buildUniformBuffer(librii::glhelper::DelegatedUBOBuilder& ubo_builder,
                          const std::map<std::string, u32>& tex_id_map,
                          u32 _mtx_id, const glm::mat4& model_matrix,
                          const glm::mat4& view_matrix,
                          const glm::mat4& proj_matrix) final;
  void buildVertexBuffer(librii::glhelper::VBOBuilder& vbo_builder) final;

private:
  lib3d::Material& mat;
  const lib3d::Polygon& poly;
  const lib3d::Bone& bone;

  const lib3d::Scene& scn;
  const lib3d::Model& mdl;

  librii::glhelper::ShaderProgram shader;

  // Only used later
  mutable u32 idx_ofs = 0;
  mutable u32 idx_size = 0;
  mutable u32 mtx_id = 0;
};

struct DrawBuffer {
  std::vector<std::unique_ptr<SceneNode>> nodes;

  auto begin() { return nodes.begin(); }
  auto begin() const { return nodes.begin(); }
  auto end() { return nodes.end(); }
  auto end() const { return nodes.end(); }

  void zSort() {
    // FIXME: Implement
  }
};

struct SceneTree {
  using Node = SceneNode;

  DrawBuffer opaque;
  DrawBuffer translucent;

  void gatherBoneRecursive(u64 boneId, const lib3d::Model& root,
                           const lib3d::Scene& scn);

  void gather(const lib3d::Model& root, const lib3d::Scene& scene) {
    if (root.getMaterials().empty() || root.getMeshes().empty() ||
        root.getBones().empty())
      return;

    // Assumes root at zero
    gatherBoneRecursive(0, root, scene);
  }
};

} // namespace riistudio::lib3d
