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
#include <plugins/gc/Export/Material.hpp>
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

void SceneImpl::prepare(SceneState& state, const kpi::INode& _host,
                        glm::mat4 v_mtx, glm::mat4 p_mtx) {
  auto& host = *dynamic_cast<const Scene*>(&_host);

  if (mImpl == nullptr) {
    mImpl = std::make_unique<Internal>();

    for (auto& model : host.getModels())
      mImpl->buildVertexBuffer(model);

    mImpl->mVboBuilder.build();
    mImpl->buildTextures(host);
  }

  for (auto& model : host.getModels())
    gather(state.getBuffers(), model, host, v_mtx, p_mtx);
}

librii::math::AABB CalcPolyBound(const lib3d::Polygon& poly,
                                 const lib3d::Bone& bone,
                                 const lib3d::Model& mdl) {
  auto mdl_mtx = bone.calcSrtMtx(&mdl);

  auto nmax = mdl_mtx * glm::vec4(poly.getBounds().max, 0.0f);
  auto nmin = mdl_mtx * glm::vec4(poly.getBounds().min, 0.0f);

  return {nmin, nmax};
}

struct Node {
  const lib3d::Scene& scene;
  const lib3d::Model& model;
  const lib3d::Bone& bone;
  const lib3d::Material& mat;
  const lib3d::Polygon& poly;
};

struct GCSceneNode : public SceneNode {
  GCSceneNode(VertexBufferTenant& tenant, librii::glhelper::VBOBuilder& v,
              const std::map<std::string, u32>& tm, Node node,
              librii::glhelper::ShaderProgram& prog, u32 mi, glm::mat4 v_mtx,
              glm::mat4 p_mtx)
      : mp_id(mi), tex_id_map(tm), mNode(node), view_matrix(v_mtx),
        proj_matrix(p_mtx) {
    this->vao_id = v.getGlId();
    this->bound = CalcPolyBound(node.poly, node.bone, node.model);

    //
    node.mat.setMegaState(this->mega_state);
    this->shader_id = prog.getId();

    // draw
    this->glBeginMode = GL_TRIANGLES;
    this->vertex_count = tenant.idx_size;
    this->glVertexDataType = GL_UNSIGNED_INT;
    this->indices = reinterpret_cast<void*>(tenant.idx_ofs * 4);

    const libcube::GCMaterialData& gc_mat =
        reinterpret_cast<const libcube::IGCMaterial&>(node.mat)
            .getMaterialData();
    for (int i = 0; i < gc_mat.samplers.size(); ++i) {
      const auto& sampler = gc_mat.samplers[i];

      TextureObj obj;

      if (sampler.mTexture.empty()) {
        // No textures specified
        continue;
      }

      {
        const auto found = tex_id_map.find(sampler.mTexture);
        if (found == tex_id_map.end()) {
          printf("Invalid texture link.\n");
          continue;
        }

        obj.active_id = i;
        obj.image_id = found->second;
      }

      obj.glMinFilter = librii::gl::gxFilterToGl(sampler.mMinFilter);
      obj.glMagFilter = librii::gl::gxFilterToGl(sampler.mMagFilter);
      obj.glWrapU = librii::gl::gxTileToGl(sampler.mWrapU);
      obj.glWrapV = librii::gl::gxTileToGl(sampler.mWrapV);

      this->texture_objects.push_back(obj);
    }
  }

  void
  buildUniformBuffer(librii::glhelper::DelegatedUBOBuilder& ubo_builder) final;

  u32 mp_id = 0;
  const std::map<std::string, u32>& tex_id_map;

private:
  // These references aren't safe, though
  Node mNode;

  glm::mat4 model_matrix{1.0f};
  glm::mat4 view_matrix;
  glm::mat4 proj_matrix;
};

void GCSceneNode::buildUniformBuffer(
    librii::glhelper::DelegatedUBOBuilder& ubo_builder) {
  mNode.mat.generateUniforms(ubo_builder, model_matrix, view_matrix,
                             proj_matrix, this->shader_id, tex_id_map,
                             mNode.poly, mNode.scene);
  mNode.mat.onSplice(ubo_builder, mNode.model, mNode.poly, mp_id);
}

void pushDisplay(VertexBufferTenant& tenant,
                 librii::glhelper::VBOBuilder& vbo_builder, Node node,
                 riistudio::lib3d::SceneBuffers& output, u32 mp_id,
                 const std::map<std::string, u32>& tex_id_map,
                 librii::glhelper::ShaderProgram& shader, glm::mat4 v_mtx,
                 glm::mat4 p_mtx) {

  auto mnode = std::make_unique<GCSceneNode>(tenant, vbo_builder, tex_id_map,
                                             node, shader, mp_id, v_mtx, p_mtx);

  auto& nodebuf = node.mat.isXluPass() ? output.translucent : output.opaque;

  nodebuf.nodes.push_back(std::move(mnode));
}

void SceneImpl::gatherBoneRecursive(SceneBuffers& output, u64 boneId,
                                    const lib3d::Model& root,
                                    const lib3d::Scene& scene, glm::mat4 v_mtx,
                                    glm::mat4 p_mtx) {
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
      Node node{
          .scene = scene,
          .model = root,
          .bone = pBone,
          .mat = mat,
          .poly = poly,
      };
      pushDisplay(mImpl->mTenants.at(mesh_name), mImpl->mVboBuilder, node,
                  output, i, mImpl->mTexIdMap,
                  mImpl->mMatToShader.at(mat.getName()).getProgram(), v_mtx,
                  p_mtx);
    }
  }

  for (u64 i = 0; i < pBone.getNumChildren(); ++i)
    gatherBoneRecursive(output, pBone.getChild(i), root, scene, v_mtx, p_mtx);
}

void SceneImpl::gather(SceneBuffers& output, const lib3d::Model& root,
                       const lib3d::Scene& scene, glm::mat4 v_mtx,
                       glm::mat4 p_mtx) {
  if (root.getMaterials().empty() || root.getMeshes().empty() ||
      root.getBones().empty())
    return;

  // Assumes root at zero
  gatherBoneRecursive(output, 0, root, scene, v_mtx, p_mtx);
}

} // namespace riistudio::lib3d
