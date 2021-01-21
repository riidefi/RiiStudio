#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "i3dmodel.hpp"
#include <algorithm>                       // std::min
#include <core/3d/gl.hpp>                  // glClearColor
#include <core/3d/renderer/SceneState.hpp> // SceneState
#include <core/util/gui.hpp>               // ImGui::GetStyle()
#include <librii/gl/Compiler.hpp>          // PacketParams
#include <librii/gl/EnumConverter.hpp>     // setGlState
#include <plugins/gc/Export/IndexedPolygon.hpp>

#include <core/3d/renderer/SceneTree.hpp>

namespace riistudio::lib3d {

glm::mat4 Bone::calcSrtMtx(const lib3d::Model* mdl) const {
  if (mdl == nullptr)
    return {};

  return calcSrtMtx(mdl->getBones());
}

SceneImpl::SceneImpl() = default;

void SceneImpl::prepare(SceneState& state, const kpi::INode& _host) {
  auto& host = *dynamic_cast<const Scene*>(&_host);

  for (auto& model : host.getModels())
    gather(state.getBuffers(), model, host);

  for (auto& tex : host.getTextures())
    state.addTexture(tex.getName(), tex);
}

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

GCSceneNode::GCSceneNode(const lib3d::Material& m, const lib3d::Polygon& p,
                         const lib3d::Bone& b, const lib3d::Scene& _scn,
                         const lib3d::Model& _mdl,
                         librii::glhelper::ShaderProgram&& prog)
    : mat(const_cast<lib3d::Material&>(m)), poly(p), bone(b), scn(_scn),
      mdl(_mdl), shader(std::move(prog)) {}

void GCSceneNode::draw(librii::glhelper::VBOBuilder& vbo_builder,
                       librii::glhelper::DelegatedUBOBuilder& ubo_builder,
                       const std::map<std::string, u32>& tex_id_map) {
  librii::gfx::MegaState state;
  mat.setMegaState(state);

  librii::gl::setGlState(state);

  // assert(mState->mVbo.VAO && node.idx_size >= 0 && node.idx_size % 3 ==
  // 0);
  glUseProgram(shader.getId());

  vbo_builder.bind();

  mat.genSamplUniforms(shader.getId(), tex_id_map);

  int i = 0;
  for (auto& splice : vbo_builder.getSplicesInRange(idx_ofs, idx_size)) {
    assert(splice.size > 0);
    librii::glhelper::DelegatedUBOBuilder PacketBuilder;

    glUniformBlockBinding(
        shader.getId(),
        glGetUniformBlockIndex(shader.getId(), "ub_PacketParams"), 2);

    int min;
    glGetActiveUniformBlockiv(shader.getId(), 2, GL_UNIFORM_BLOCK_DATA_SIZE,
                              &min);
    PacketBuilder.setBlockMin(2, min);

    mat.onSplice(PacketBuilder, mdl, poly, i);
    // TODO: huge hack, very expensive
    PacketBuilder.submit();

    ubo_builder.use(mtx_id);
    PacketBuilder.use(0);

    librii::gl::PacketParams pack{};
    for (auto& p : pack.posMtx)
      p = glm::transpose(glm::mat4{1.0f});

    assert(dynamic_cast<const libcube::IndexedPolygon*>(&poly) != nullptr);
    const auto& ipoly = reinterpret_cast<const libcube::IndexedPolygon&>(poly);

    if (poly.isVisible()) {
      const auto mtx =
          ipoly.getPosMtx(reinterpret_cast<const libcube::Model&>(mdl), i);
      for (int p = 0; p < std::min(static_cast<std::size_t>(10), mtx.size());
           ++p) {
        pack.posMtx[p] = glm::transpose(mtx[p]);
      }

      if (poly.isVisible())
        glDrawElements(GL_TRIANGLES, splice.size, GL_UNSIGNED_INT,
                       (void*)(splice.offset * 4));
    }
    ++i;
  }
}

void GCSceneNode::expandBound(AABB& bound) {
  auto mdl_mtx = bone.calcSrtMtx(&mdl);

  auto nmax = mdl_mtx * glm::vec4(poly.getBounds().max, 0.0f);
  auto nmin = mdl_mtx * glm::vec4(poly.getBounds().min, 0.0f);

  riistudio::lib3d::AABB newBound{nmin, nmax};
  bound.expandBound(newBound);
}

void GCSceneNode::buildUniformBuffer(
    librii::glhelper::DelegatedUBOBuilder& ubo_builder,
    const std::map<std::string, u32>& tex_id_map, u32 _mtx_id,
    const glm::mat4& model_matrix, const glm::mat4& view_matrix,
    const glm::mat4& proj_matrix) {
  // no more mdl here, we just use PNMTX0
  // mdl = computeBoneMdl(node->bone);
  // (Up to the mat)
  // if (!node->poly.hasAttrib(lib3d::Polygon::SimpleAttrib::EnvelopeIndex))
  // {
  //  mdl = ((lib3d::Bone&)node->bone).calcSrtMtx();
  //}

  mat.generateUniforms(ubo_builder, model_matrix, view_matrix, proj_matrix,
                       shader.getId(), tex_id_map, poly, scn);
  mtx_id = _mtx_id;
}

void GCSceneNode::update(lib3d::Material* _mat) {
  DebugReport("Recompiling shader for %s..\n", _mat->getName().c_str());
  mat = *_mat;
  const auto shader_sources = mat.generateShaders();
  librii::glhelper::ShaderProgram new_shader(
      shader_sources.first,
      _mat->applyCacheAgain ? _mat->cachedPixelShader : shader_sources.second);
  if (new_shader.getError()) {
    _mat->isShaderError = true;
    _mat->shaderError = new_shader.getErrorDesc();
    return;
  }
  shader = std::move(new_shader);
  _mat->isShaderError = false;
}

void GCSceneNode::buildVertexBuffer(librii::glhelper::VBOBuilder& vbo_builder) {
  idx_ofs = static_cast<u32>(vbo_builder.mIndices.size());
  poly.propogate(mdl, vbo_builder);
  idx_size = static_cast<u32>(vbo_builder.mIndices.size()) - idx_ofs;
}

void SceneImpl::gatherBoneRecursive(SceneBuffers& output, u64 boneId,
                                    const lib3d::Model& root,
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
    GCSceneNode node{
        mats[display.matId], polys[display.polyId], pBone, scene, root,
        std::move(shader)};

    auto& nodebuf =
        mats[display.matId].isXluPass() ? output.translucent : output.opaque;

    nodebuf.nodes.push_back(std::make_unique<GCSceneNode>(std::move(node)));
    mats[display.matId].observers.push_back(nodebuf.nodes.back().get());
  }

  for (u64 i = 0; i < pBone.getNumChildren(); ++i)
    gatherBoneRecursive(output, pBone.getChild(i), root, scene);
}

void SceneImpl::gather(SceneBuffers& output, const lib3d::Model& root,
                       const lib3d::Scene& scene) {
  if (root.getMaterials().empty() || root.getMeshes().empty() ||
      root.getBones().empty())
    return;

  // Assumes root at zero
  gatherBoneRecursive(output, 0, root, scene);
}

} // namespace riistudio::lib3d
