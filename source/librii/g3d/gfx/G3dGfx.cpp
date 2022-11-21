#include "G3dGfx.hpp"

// OPENGL
#include <core/3d/gl.hpp>

#include <librii/image/CheckerBoard.hpp>

namespace librii::g3d::gfx {

using namespace riistudio;

//
// Render Data
//

struct Node {
  const lib3d::Scene& scene;
  const lib3d::Model& model;
  const lib3d::Bone& bone;
  const lib3d::Material& mat;
  const lib3d::Polygon& poly;
};
template <typename T>
librii::gfx::SceneNode::UniformData pushUniform(u32 binding_point,
                                                const T& data) {
  const u8* pack_begin = reinterpret_cast<const u8*>(&data);
  const u8* pack_end = reinterpret_cast<const u8*>(pack_begin + sizeof(data));

  return {.binding_point = binding_point, .raw_data = {pack_begin, pack_end}};
}
using SceneNode = librii::gfx::SceneNode;

static constexpr librii::image::NullTextureData<64, 64> NullCheckerboard;
struct MyDefTex : public librii::image::NullTexture<64, 64> {
  using NullTexture::NullTexture;
  std::string getName() const override { return "<DEFAULT TEXTURE>"; }
};
static MyDefTex DefaultTex(NullCheckerboard);

void MakeSceneNode(SceneNode& out, lib3d::IndexRange tenant,
                   librii::glhelper::VBOBuilder& v, G3dTextureCache& tex_id_map,
                   Node node, librii::glhelper::ShaderProgram& prog, u32 mp_id,
                   glm::mat4 view_matrix, glm::mat4 proj_matrix) {
  glm::mat4 model_matrix{1.0f};

  out.vao_id = v.getGlId();
  out.bound = lib3d::CalcPolyBound(node.poly, node.bone, node.model);

  //
  node.mat.setMegaState(out.mega_state);
  out.shader_id = prog.getId();

  // draw
  out.primitive_type = librii::gfx::PrimitiveType::Triangles;
  out.vertex_count = tenant.size;
  out.vertex_data_type = librii::gfx::DataType::U32;
  out.indices = reinterpret_cast<void*>(tenant.start * sizeof(u32));

  const libcube::GCMaterialData& gc_mat =
      reinterpret_cast<const libcube::IGCMaterial&>(node.mat).getMaterialData();
  for (int i = 0; i < gc_mat.samplers.size(); ++i) {
    const auto& sampler = gc_mat.samplers[i];

    librii::gfx::TextureObj obj;

    if (sampler.mTexture.empty()) {
      // No textures specified
      continue;
    }

    {
      const auto found = tex_id_map.getCachedTexture(sampler.mTexture);
      if (!found) {
        printf("Invalid texture link.\n");
        if (!tex_id_map.isCached(DefaultTex)) {
          tex_id_map.cache(DefaultTex);
        }
        obj.active_id = i;
        obj.image_id = tex_id_map.getCachedTexture(DefaultTex);
      } else {
        obj.active_id = i;
        obj.image_id = *found;
      }
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

    scene.projection = proj_matrix * view_matrix * model_matrix;
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
          data.texMatrices[i].compute(model_matrix, proj_matrix * view_matrix));
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
        out.shader_id, glGetUniformBlockIndex(out.shader_id, "ub_SceneParams"),
        0);
    glUniformBlockBinding(
        out.shader_id,
        glGetUniformBlockIndex(out.shader_id, "ub_MaterialParams"), 1);
    glUniformBlockBinding(
        out.shader_id, glGetUniformBlockIndex(out.shader_id, "ub_PacketParams"),
        2);
#endif // __EMSCRIPTEN__

    const s32 samplerIds[] = {0, 1, 2, 3, 4, 5, 6, 7};
#ifdef RII_GL
    glUseProgram(out.shader_id);
    u32 uTexLoc = glGetUniformLocation(out.shader_id, "u_Texture");
    glUniform1iv(uTexLoc, 8, samplerIds);
#endif
  }
}

void pushDisplay(lib3d::IndexRange tenant,
                 librii::glhelper::VBOBuilder& vbo_builder, Node node,
                 riistudio::lib3d::SceneBuffers& output, u32 mp_id,
                 G3dTextureCache& tex_id_map,
                 librii::glhelper::ShaderProgram& shader, glm::mat4 v_mtx,
                 glm::mat4 p_mtx) {

  librii::gfx::SceneNode mnode;
  MakeSceneNode(mnode, tenant, vbo_builder, tex_id_map, node, shader, mp_id,
                v_mtx, p_mtx);

  auto& nodebuf = node.mat.isXluPass() ? output.translucent : output.opaque;

  nodebuf.nodes.push_back(std::move(mnode));
}

template <typename ModelType>
void gatherBoneRecursive(lib3d::SceneBuffers& output, u64 boneId,
                         const ModelType& root, const lib3d::Scene& scene,
                         glm::mat4 v_mtx, glm::mat4 p_mtx,
                         G3dSceneRenderData& render_data) {
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

    // REASON FOR REMOVAL: We will already create the shader if we cache miss
    //
    // if (!render_data.mMaterialData.isShaderCached(mat)) {
    //   render_data.mMaterialData.cacheShader(mat);
    // }

    for (u32 i = 0; i < poly.getMeshData().mMatrixPrimitives.size(); ++i) {
      if (!poly.isVisible())
        continue;

      DrawCallPath mesh_name{
          .model_name = "TODO", .mesh_name = poly.getName(), .mprim_index = i};
      Node node{
          .scene = scene,
          .model = root,
          .bone = pBone,
          .mat = mat,
          .poly = poly,
      };
      pushDisplay(render_data.mVertexRenderData.getDrawCallVertices(mesh_name),
                  render_data.mVertexRenderData.mVboBuilder, node, output, i,
                  render_data.mTextureData,
                  render_data.mMaterialData.getCachedShader(mat), v_mtx, p_mtx);
    }
  }

  for (u64 i = 0; i < pBone.getNumChildren(); ++i)
    gatherBoneRecursive(output, pBone.getChild(i), root, scene, v_mtx, p_mtx,
                        render_data);
}

template <typename ModelType>
void gather(lib3d::SceneBuffers& output, const ModelType& root,
            const lib3d::Scene& scene, glm::mat4 v_mtx, glm::mat4 p_mtx,
            G3dSceneRenderData& render_data) {
  if (root.getMaterials().empty() || root.getMeshes().empty() ||
      root.getBones().empty())
    return;

  // Assumes root at zero
  gatherBoneRecursive(output, 0, root, scene, v_mtx, p_mtx, render_data);
}

std::unique_ptr<G3dSceneRenderData>
G3DSceneCreateRenderData(riistudio::g3d::Collection& scene) {
  auto result = std::make_unique<G3dSceneRenderData>();
  result->init(scene);
  return result;
}

// This code is shared between J3D and G3D right now
void G3DSceneAddNodesToBuffer(riistudio::lib3d::SceneState& state,
                              const riistudio::g3d::Collection& scene,
                              glm::mat4 v_mtx, glm::mat4 p_mtx,
                              G3dSceneRenderData& render_data) {
  // REASON FOR REMOVAL: Using the renderer in this file, now
  // scene.prepare(state, scene, v_mtx, p_mtx);

  // REASON FOR REMOVAL: User must manually initialize the render data
  // render_data.init(scene);

  // Reupload changed textures
  render_data.mTextureData.update(scene);

  for (auto& model : scene.getModels())
    gather(state.getBuffers(), model, scene, v_mtx, p_mtx, render_data);
}

void Any3DSceneAddNodesToBuffer(riistudio::lib3d::SceneState& state,
                                const lib3d::Scene& scene, glm::mat4 v_mtx,
                                glm::mat4 p_mtx,
                                G3dSceneRenderData& render_data) {
  // Reupload changed textures
  render_data.mTextureData.update(scene);

  for (auto& model : scene.getModels())
    gather(state.getBuffers(), model, scene, v_mtx, p_mtx, render_data);
}

} // namespace librii::g3d::gfx
