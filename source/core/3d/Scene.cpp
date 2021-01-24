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

struct VertexBufferTenant {
  VertexBufferTenant(librii::glhelper::VBOBuilder& vbo_builder,
                     const lib3d::Model& mdl,
                     const libcube::IndexedPolygon& poly, u32 mp_id) {
    idx_ofs = static_cast<u32>(vbo_builder.mIndices.size());
    poly.propagate(mdl, mp_id, vbo_builder);
    idx_size = static_cast<u32>(vbo_builder.mIndices.size()) - idx_ofs;
  }
  u32 idx_ofs;
  u32 idx_size;
};

struct MeshName {
  std::string string;
  u32 mprim_index = 0;

  bool operator==(const MeshName&) const = default;
};

class MeshHash {
public:
  std::size_t operator()(const MeshName& name) const {
    return std::hash<std::string>()(name.string) ^ name.mprim_index;
  }
};
struct SceneImpl::Internal {
  librii::glhelper::VBOBuilder mVboBuilder;
  // Maps mesh names -> slots of mVboBuilder
  std::unordered_map<MeshName, VertexBufferTenant, MeshHash> mTenants;

  std::vector<GlTexture> mTextures;
  // Maps texture names -> slots of mTextures
  std::map<std::string, u32> mTexIdMap;

  // Maps material name -> Shader
  // Each entry is heap allocated so we shouldnt have to worry about dangling
  // references.
  std::map<std::string, librii::glhelper::ShaderProgram> mMatToShader;

  void buildVertexBuffer(const Model& model) {
    for (auto& mesh : model.getMeshes()) {
      auto& gc_mesh = reinterpret_cast<const libcube::IndexedPolygon&>(mesh);

      for (u32 i = 0; i < gc_mesh.getMeshData().mMatrixPrimitives.size(); ++i) {
        const MeshName mesh_name{.string = mesh.getName(), .mprim_index = i};

        if (mTenants.contains(mesh_name))
          continue;

        VertexBufferTenant tenant{mVboBuilder, model, gc_mesh, i};
        mTenants.emplace(mesh_name, tenant);
      }
    }
  }

  void buildTextures(const Scene& host) {
    for (auto& tex : host.getTextures()) {
      if (mTexIdMap.contains(tex.getName()))
        continue;

      mTexIdMap[tex.getName()] = mTextures.emplace_back(tex).getGlId();
    }
  }
};

SceneImpl::SceneImpl() = default;
SceneImpl::~SceneImpl() = default;

void SceneImpl::prepare(SceneState& state, const kpi::INode& _host) {
  auto& host = *dynamic_cast<const Scene*>(&_host);

  if (mImpl == nullptr) {
    mImpl = std::make_unique<Internal>();

    for (auto& model : host.getModels())
      mImpl->buildVertexBuffer(model);

    mImpl->mVboBuilder.build();
    mImpl->buildTextures(host);
  }

  for (auto& model : host.getModels())
    gather(state.getBuffers(), model, host);
}

struct GCSceneNode : public SceneNode {
  VertexBufferTenant& mVertexBufferTenant;
  GCSceneNode(VertexBufferTenant& tenant, librii::glhelper::VBOBuilder& v,
              const std::map<std::string, u32>& tm, const lib3d::Material& m,
              const lib3d::Polygon& p, const lib3d::Bone& b,
              const lib3d::Scene& _scn, const lib3d::Model& _mdl,
              librii::glhelper::ShaderProgram& prog, u32 mi)
      : mVertexBufferTenant(tenant), mp_id(mi), tex_id_map(tm), vbo_builder(v),
        mat(const_cast<lib3d::Material&>(m)), poly(p), bone(b), scn(_scn),
        mdl(_mdl), shader(prog) {}

  void draw(librii::glhelper::DelegatedUBOBuilder& ubo_builder,
            u32 mtx_id) final;
  void expandBound(AABB& bound) final;

  void update(lib3d::Material* _mat) final;

  void buildUniformBuffer(librii::glhelper::DelegatedUBOBuilder& ubo_builder,
                          const glm::mat4& model_matrix,
                          const glm::mat4& view_matrix,
                          const glm::mat4& proj_matrix) final;

  u32 mp_id = 0;
  const std::map<std::string, u32>& tex_id_map;

private:
  // Refers to the scene node's VBOBuilder;
  // that instance is a unique_ptr, and the scene must outlive its children, so
  // we can be safe about this reference dangling.
  librii::glhelper::VBOBuilder& vbo_builder;

  // These references aren't safe, though
  lib3d::Material& mat;
  const lib3d::Polygon& poly;
  const lib3d::Bone& bone;

  const lib3d::Scene& scn;
  const lib3d::Model& mdl;

  librii::glhelper::ShaderProgram& shader;
};

void GCSceneNode::draw(librii::glhelper::DelegatedUBOBuilder& ubo_builder,
                       u32 mtx_id) {
  librii::gfx::MegaState state;
  mat.setMegaState(state);

  librii::gl::setGlState(state);

  // assert(mState->mVbo.VAO && node.idx_size >= 0 && node.idx_size % 3 ==
  // 0);
  glUseProgram(shader.getId());

  vbo_builder.bind();
  ubo_builder.use(mtx_id);
  mat.genSamplUniforms(shader.getId(), tex_id_map);

  if (poly.isVisible())
    glDrawElements(GL_TRIANGLES, mVertexBufferTenant.idx_size, GL_UNSIGNED_INT,
                   reinterpret_cast<void*>(mVertexBufferTenant.idx_ofs * 4));
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
    const glm::mat4& model_matrix, const glm::mat4& view_matrix,
    const glm::mat4& proj_matrix) {
  mat.generateUniforms(ubo_builder, model_matrix, view_matrix, proj_matrix,
                       shader.getId(), tex_id_map, poly, scn);
  mat.onSplice(ubo_builder, mdl, poly, mp_id);
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

void pushDisplay(
    VertexBufferTenant& tenant, librii::glhelper::VBOBuilder& vbo_builder,
    const riistudio::lib3d::Material& mat, const libcube::IndexedPolygon& poly,
    const riistudio::lib3d::Bone& pBone, const riistudio::lib3d::Scene& scene,
    const riistudio::lib3d::Model& root, riistudio::lib3d::SceneBuffers& output,
    u32 mp_id, const std::map<std::string, u32>& tex_id_map,
    librii::glhelper::ShaderProgram& shader) {

  auto node =
      std::make_unique<GCSceneNode>(tenant, vbo_builder, tex_id_map, mat, poly,
                                    pBone, scene, root, shader, mp_id);

  auto& nodebuf = mat.isXluPass() ? output.translucent : output.opaque;

  nodebuf.nodes.push_back(std::move(node));
  mat.observers.push_back(nodebuf.nodes.back().get());
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
    assert(display.matId < mats.size());
    assert(display.polyId < polys.size());
    const auto& mat = mats[display.matId];
    const auto& poly =
        reinterpret_cast<const libcube::IndexedPolygon&>(polys[display.polyId]);

    if (!mImpl->mMatToShader.contains(mat.getName())) {
      const auto shader_sources = mat.generateShaders();

      mImpl->mMatToShader.emplace(
          mat.getName(), librii::glhelper::ShaderProgram{
                             shader_sources.first, shader_sources.second});
    }

    for (u32 i = 0; i < poly.getMeshData().mMatrixPrimitives.size(); ++i) {
      MeshName mesh_name{.string = poly.getName(), .mprim_index = i};
      pushDisplay(mImpl->mTenants.at(mesh_name), mImpl->mVboBuilder, mat, poly,
                  pBone, scene, root, output, i, mImpl->mTexIdMap,
                  mImpl->mMatToShader.at(mat.getName()));
    }
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
