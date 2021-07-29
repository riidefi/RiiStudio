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
#include <librii/glhelper/UBOBuilder.hpp>
#include <librii/mtx/TexMtx.hpp>
#include <plugins/gc/Export/IndexedPolygon.hpp>
#include <plugins/gc/Export/Material.hpp>
#include <unordered_map>

namespace riistudio::lib3d {

glm::mat4 calcSrtMtx(const Bone& bone, const lib3d::Model* mdl) {
  if (mdl == nullptr)
    return {};

  return calcSrtMtx(bone, mdl->getBones());
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
  auto mdl_mtx = calcSrtMtx(bone, &mdl);

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
template <typename T>
SceneNode::UniformData pushUniform(u32 binding_point, const T& data) {
  const u8* pack_begin = reinterpret_cast<const u8*>(&data);
  const u8* pack_end = reinterpret_cast<const u8*>(pack_begin + sizeof(data));

  return SceneNode::UniformData{.binding_point = binding_point,
                                .raw_data = {pack_begin, pack_end}};
}
void MakeSceneNode(SceneNode& out, VertexBufferTenant& tenant,
                   librii::glhelper::VBOBuilder& v,
                   const std::map<std::string, u32>& tex_id_map, Node node,
                   librii::glhelper::ShaderProgram& prog, u32 mp_id,
                   glm::mat4 view_matrix, glm::mat4 proj_matrix) {
  glm::mat4 model_matrix{1.0f};

  out.vao_id = v.getGlId();
  out.bound = CalcPolyBound(node.poly, node.bone, node.model);

  //
  node.mat.setMegaState(out.mega_state);
  out.shader_id = prog.getId();

  // draw
#ifdef RII_GL
  out.glBeginMode = GL_TRIANGLES;
#endif
  out.vertex_count = tenant.idx_size;
#ifdef RII_GL
  out.glVertexDataType = GL_UNSIGNED_INT;
#endif
  out.indices = reinterpret_cast<void*>(tenant.idx_ofs * 4);

  const libcube::GCMaterialData& gc_mat =
      reinterpret_cast<const libcube::IGCMaterial&>(node.mat).getMaterialData();
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

    out.texture_objects.push_back(obj);
  }

  {
    int query_min;

    for (u32 i = 0; i < 3; ++i) {
#ifdef RII_GL
      glGetActiveUniformBlockiv(out.shader_id, i, GL_UNIFORM_BLOCK_DATA_SIZE,
                                &query_min);
#endif
      out.uniform_mins.push_back(
          {.binding_point = i, .min_size = static_cast<u32>(query_min)});
    }
  }

  {
    librii::gl::UniformSceneParams scene;

    scene.projection = model_matrix * view_matrix * proj_matrix;
    scene.Misc0 = {};

    out.uniform_data.emplace_back(pushUniform(0, scene));
  }

  {
    const auto& data = reinterpret_cast<const libcube::IGCMaterial&>(node.mat)
                           .getMaterialData();

    librii::gl::UniformMaterialParams tmp{};
    librii::gl::setUniformsFromMaterial(tmp, data);

    for (int i = 0; i < data.texMatrices.size(); ++i) {
      tmp.TexMtx[i] = glm::transpose(
          data.texMatrices[i].compute(model_matrix, view_matrix * proj_matrix));
    }
    for (int i = 0; i < data.samplers.size(); ++i) {
      if (data.samplers[i].mTexture.empty())
        continue;
      const auto* texData =
          reinterpret_cast<const libcube::IGCMaterial&>(node.mat).getTexture(
              reinterpret_cast<const libcube::Scene&>(node.scene),
              data.samplers[i].mTexture);
      if (texData == nullptr)
        continue;
      tmp.TexParams[i] = glm::vec4{texData->getWidth(), texData->getHeight(), 0,
                                   data.samplers[i].mLodBias};
    }

    out.uniform_data.push_back(pushUniform(1, tmp));
  }

  {
    // builder.reset(2);
    librii::gl::PacketParams pack{};
    for (auto& p : pack.posMtx)
      p = glm::transpose(glm::mat4{1.0f});

    assert(dynamic_cast<const libcube::IndexedPolygon*>(&node.poly) != nullptr);
    const auto& ipoly =
        reinterpret_cast<const libcube::IndexedPolygon&>(node.poly);

    const auto mtx = ipoly.getPosMtx(
        reinterpret_cast<const libcube::Model&>(node.model), mp_id);
    for (int p = 0; p < std::min<std::size_t>(10, mtx.size()); ++p) {
      pack.posMtx[p] = glm::transpose(mtx[p]);
    }

    out.uniform_data.push_back(pushUniform(2, pack));
  }

  {

    // WebGL doesn't support binding=n in the shader
#if defined(__EMSCRIPTEN__) || defined(__APPLE__)
    glUniformBlockBinding(
        out.shader_id, glGetUniformBlockIndex(out.shader_id, "ub_SceneParams"), 0);
    glUniformBlockBinding(
        out.shader_id, glGetUniformBlockIndex(out.shader_id, "ub_MaterialParams"), 1);
    glUniformBlockBinding(
        out.shader_id, glGetUniformBlockIndex(out.shader_id, "ub_PacketParams"), 2);
#endif // __EMSCRIPTEN__

    const s32 samplerIds[] = {0, 1, 2, 3, 4, 5, 6, 7};
#ifdef RII_GL
    glUseProgram(out.shader_id);
    u32 uTexLoc = glGetUniformLocation(out.shader_id, "u_Texture");
    glUniform1iv(uTexLoc, 8, samplerIds);
#endif
  }
}

void pushDisplay(VertexBufferTenant& tenant,
                 librii::glhelper::VBOBuilder& vbo_builder, Node node,
                 riistudio::lib3d::SceneBuffers& output, u32 mp_id,
                 const std::map<std::string, u32>& tex_id_map,
                 librii::glhelper::ShaderProgram& shader, glm::mat4 v_mtx,
                 glm::mat4 p_mtx) {

  SceneNode mnode;
  MakeSceneNode(mnode, tenant, vbo_builder, tex_id_map, node, shader, mp_id,
                v_mtx, p_mtx);

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
