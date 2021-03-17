#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "i3dmodel.hpp"
#include <algorithm>                       // std::min
#include <core/3d/gl.hpp>                  // glClearColor
#include <core/3d/renderer/SceneState.hpp> // SceneState
#include <core/3d/renderer/SceneTree.hpp>
#include <core/util/gui.hpp>           // ImGui::GetStyle()
#include <librii/gl/Compiler.hpp>      // PacketParams
#include <librii/gl/EnumConverter.hpp> // setGlState
#include <plugins/gc/Export/IndexedPolygon.hpp>
#include <unordered_map>

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

struct ShaderUser {
  ShaderUser(librii::glhelper::ShaderProgram&& shader) {
    mImpl = std::make_unique<Impl>(std::move(shader));
  }

  auto& getProgram() { return mImpl->mProgram; }
  void attachToMaterial(const lib3d::Material& mat) {
    mat.observers.push_back(mImpl.get());
  }

private:
  // IObservers should be heap allocated
  struct Impl : public IObserver {
    Impl(librii::glhelper::ShaderProgram&& program)
        : mProgram(std::move(program)) {}

    librii::glhelper::ShaderProgram mProgram;

    void update(lib3d::Material* _mat) final {
      DebugReport("Recompiling shader for %s..\n", _mat->getName().c_str());
      const auto shader_sources = _mat->generateShaders();
      librii::glhelper::ShaderProgram new_shader(
          shader_sources.first, _mat->applyCacheAgain ? _mat->cachedPixelShader
                                                      : shader_sources.second);
      if (new_shader.getError()) {
        _mat->isShaderError = true;
        _mat->shaderError = new_shader.getErrorDesc();
        return;
      }
      mProgram = std::move(new_shader);
      _mat->isShaderError = false;
    }
  };
  std::unique_ptr<Impl> mImpl;
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
  std::map<std::string, ShaderUser> mMatToShader;

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
      : mVertexBufferTenant(tenant), mp_id(mi), tex_id_map(tm),
        mVaoId(v.getGlId()), mat(m), poly(p), bone(b), scn(_scn), mdl(_mdl) {
    // Calc the bound
    {
      auto mdl_mtx = bone.calcSrtMtx(&mdl);

      auto nmax = mdl_mtx * glm::vec4(poly.getBounds().max, 0.0f);
      auto nmin = mdl_mtx * glm::vec4(poly.getBounds().min, 0.0f);

      mBound = {nmin, nmax};
    }

    //
    mat.setMegaState(mState);
    mShaderId = prog.getId();
  }

  void draw(librii::glhelper::DelegatedUBOBuilder& ubo_builder,
            u32 mtx_id) final;
  void expandBound(librii::math::AABB& bound) final {
    bound.expandBound(mBound);
  }

  void buildUniformBuffer(librii::glhelper::DelegatedUBOBuilder& ubo_builder,
                          const glm::mat4& model_matrix,
                          const glm::mat4& view_matrix,
                          const glm::mat4& proj_matrix) final;

  u32 mp_id = 0;
  const std::map<std::string, u32>& tex_id_map;

private:
  u32 mVaoId = 0;

  // These references aren't safe, though
  const lib3d::Material& mat;
  const lib3d::Polygon& poly;
  const lib3d::Bone& bone;

  const lib3d::Scene& scn;
  const lib3d::Model& mdl;

  librii::math::AABB mBound;
  librii::gfx::MegaState mState;
  u32 mShaderId = 0;
};

void GCSceneNode::draw(librii::glhelper::DelegatedUBOBuilder& ubo_builder,
                       u32 mtx_id) {
  librii::gl::setGlState(mState);
  glUseProgram(mShaderId);
  glBindVertexArray(mVaoId);
  ubo_builder.use(mtx_id);
  mat.genSamplUniforms(mShaderId, tex_id_map);

  glDrawElements(GL_TRIANGLES, mVertexBufferTenant.idx_size, GL_UNSIGNED_INT,
                 reinterpret_cast<void*>(mVertexBufferTenant.idx_ofs * 4));
}

void GCSceneNode::buildUniformBuffer(
    librii::glhelper::DelegatedUBOBuilder& ubo_builder,
    const glm::mat4& model_matrix, const glm::mat4& view_matrix,
    const glm::mat4& proj_matrix) {
  mat.generateUniforms(ubo_builder, model_matrix, view_matrix, proj_matrix,
                       mShaderId, tex_id_map, poly, scn);
  mat.onSplice(ubo_builder, mdl, poly, mp_id);
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
      mImpl->mMatToShader.at(mat.getName()).attachToMaterial(mat);
    }

    for (u32 i = 0; i < poly.getMeshData().mMatrixPrimitives.size(); ++i) {
      if (!poly.isVisible())
        continue;

      MeshName mesh_name{.string = poly.getName(), .mprim_index = i};
      pushDisplay(mImpl->mTenants.at(mesh_name), mImpl->mVboBuilder, mat, poly,
                  pBone, scene, root, output, i, mImpl->mTexIdMap,
                  mImpl->mMatToShader.at(mat.getName()).getProgram());
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
