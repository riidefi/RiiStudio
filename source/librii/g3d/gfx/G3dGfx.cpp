#include "G3dGfx.hpp"

// OPENGL
#include <core/3d/gl.hpp>

#include <librii/image/CheckerBoard.hpp>

// TRAITS FOR WIITRIG
librii::math::SRT3 getSrt(const libcube::IBoneDelegate& bone) {
  return bone.getSRT();
}
s32 parentOf(const libcube::IBoneDelegate& bone) {
  return bone.getBoneParent();
}
bool ssc(const libcube::IBoneDelegate& bone) { return bone.getSSC(); }

#include <librii/g3d/io/WiiTrig.hpp>

namespace librii::g3d::gfx {

using namespace riistudio;

//
// Render Data
//

struct Node {
  ModelView model;
  const lib3d::Bone& bone;
  const libcube::IGCMaterial& mat;
  const libcube::IndexedPolygon& poly;
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

Result<std::vector<glm::mat4>> getPosMtx(const libcube::IndexedPolygon& p,
                                         ModelView model, u64 mpid) {
  std::vector<glm::mat4> out;

  const auto& mp = p.getMeshData().mMatrixPrimitives[mpid];

  struct Bones_ {
    size_t size() const { return m.bones.size(); }
    libcube::IBoneDelegate& operator[](size_t i) {
      return const_cast<libcube::IBoneDelegate&>(*m.bones[i]);
    }
    ModelView m;
  };
  Bones_ bones{model};

  const auto handle_drw = [&](const libcube::DrawMatrix& drw) -> Result<void> {
    glm::mat4x4 curMtx(0.0f);

    // Rigid -- bone space
    if (drw.mWeights.size() == 1) {
      u32 boneID = drw.mWeights[0].boneId;
      EXPECT(boneID < model.bones.size());
      curMtx = calcSrtMtx(*model.bones[boneID], bones,
                          librii::g3d::ScalingRule::Maya);
    } else {
      // already world space
      curMtx = glm::mat4x4(1.0f);
    }
    out.push_back(curMtx);
    return {};
  };

  if (mp.mDrawMatrixIndices.empty()) {
    // TODO: For G3D this isn't configurable per-mp
    if (model.drawMatrices.size() > mp.mCurrentMatrix) {
      TRY(handle_drw(model.drawMatrices[mp.mCurrentMatrix]));
    } else {
      // todo: is there really no rigging info here..?
      libcube::DrawMatrix tmp;
      tmp.mWeights.emplace_back(0, 1.0f);
      TRY(handle_drw(tmp));
    }
  } else {
    for (const auto it : mp.mDrawMatrixIndices) {
      EXPECT(it < model.drawMatrices.size());
      TRY(handle_drw(model.drawMatrices[it]));
    }
  }
  return out;
}

Result<void> MakeSceneNode(SceneNode& out, lib3d::IndexRange tenant,
                           librii::glhelper::VBOBuilder& v,
                           G3dTextureCache& tex_id_map, Node node,
                           librii::glhelper::ShaderProgram& prog, u32 mp_id,
                           glm::mat4 view_matrix, glm::mat4 proj_matrix,
                           std::string& err) {
  glm::mat4 model_matrix{1.0f};

  out.vao_id = v.getGlId();
  out.bound = {};
  // lib3d::CalcPolyBound(node.poly, node.bone, node.model);

  //
  out.mega_state = TRY(node.mat.setMegaState());
  out.shader_id = prog.getId();

  // draw
  out.primitive_type = librii::gfx::PrimitiveType::Triangles;
  out.vertex_count = tenant.size;
  out.vertex_data_type = librii::gfx::DataType::U32;
  out.indices = reinterpret_cast<void*>(tenant.start * sizeof(u32));

  const libcube::GCMaterialData& gc_mat = node.mat.getMaterialData();
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
        err = std::format("Cannot find texture \"{}\"", sampler.mTexture);
        if (!tex_id_map.isCached(DefaultTex)) {
          tex_id_map.cache(DefaultTex);
        }
        obj.active_id = i;
        obj.image_id = TRY(tex_id_map.getCachedTexture(DefaultTex));
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
    const auto& data = node.mat.getMaterialData();

    librii::gl::UniformMaterialParams tmp{};
    librii::gl::setUniformsFromMaterial(tmp, data);

    for (int i = 0; i < data.texMatrices.size(); ++i) {
      tmp.TexMtx[i] = glm::transpose(TRY(data.texMatrices[i].compute(
          model_matrix, proj_matrix * view_matrix)));
    }
    for (int i = 0; i < data.samplers.size(); ++i) {
      if (data.samplers[i].mTexture.empty())
        continue;
      auto tex = data.samplers[i].mTexture;
      const libcube::Texture* texData = nullptr;
      for (auto* x : node.model.textures) {
        if (x->getName() == tex) {
          texData = x;
          break;
        }
      }
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

    const auto mtx = TRY(getPosMtx(node.poly, node.model, mp_id));
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

  return {};
}

void pushDisplay(lib3d::IndexRange tenant,
                 librii::glhelper::VBOBuilder& vbo_builder, Node node,
                 riistudio::lib3d::SceneBuffers& output, u32 mp_id,
                 G3dTextureCache& tex_id_map,
                 librii::glhelper::ShaderProgram& shader, glm::mat4 v_mtx,
                 glm::mat4 p_mtx, std::string& err) {

  librii::gfx::SceneNode mnode;
  auto err_ = MakeSceneNode(mnode, tenant, vbo_builder, tex_id_map, node,
                            shader, mp_id, v_mtx, p_mtx, err);
  if (!err_.has_value()) {
    err = err + "\n" + err_.error();
    return;
  }

  auto& nodebuf = node.mat.isXluPass() ? output.translucent : output.opaque;

  nodebuf.nodes.push_back(std::move(mnode));
}

Result<void> gatherBoneRecursive(lib3d::SceneBuffers& output, u64 boneId,
                                 ModelView view, glm::mat4 v_mtx,
                                 glm::mat4 p_mtx,
                                 G3dSceneRenderData& render_data,
                                 std::string& err, lib3d::RenderType type) {
  if (boneId >= view.bones.size()) {
    return std::unexpected("Invalid bone id");
  }
  const auto& pBone = *view.bones[boneId];
  const u64 nDisplay = pBone.getNumDisplays();

  for (u64 i = 0; i < nDisplay; ++i) {
    const auto display = pBone.getDisplay(i);
    if (display.matId >= view.mats.size()) {
      return std::unexpected("Invalid material ID");
    }
    if (display.polyId >= view.polys.size()) {
      return std::unexpected("Invalid polygon ID");
    }
    const auto& mat = *view.mats[display.matId];
    const auto& poly = *view.polys[display.polyId];

    // REASON FOR REMOVAL: We will already create the shader if we cache miss
    //
    // if (!render_data.mMaterialData.isShaderCached(mat)) {
    //   render_data.mMaterialData.cacheShader(mat);
    // }

    for (u32 i = 0; i < poly.getMeshData().mMatrixPrimitives.size(); ++i) {
      if (!poly.isVisible())
        continue;

      DrawCallPath mesh_name{.model_name = std::to_string(view.model_id),
                             .mesh_name = poly.getName(),
                             .mprim_index = i};
      Node node{
          .model = view,
          .bone = pBone,
          .mat = mat,
          .poly = poly,
      };
      auto shader = render_data.mMaterialData.getCachedShader(mat, type);
      if (!shader) {
        err += std::format("\nInvalid shader for material {}: {}",
                           mat.getName(), shader.error());
        continue;
      }
      assert(*shader && "getCachedShader() should never return nullptr");
      std::string _err;
      pushDisplay(
          TRY(render_data.mVertexRenderData.getDrawCallVertices(mesh_name)),
          render_data.mVertexRenderData.mVboBuilder, node, output, i,
          render_data.mTextureData, **shader, v_mtx, p_mtx, _err);
      if (_err.size()) {
        err = err + "\n" + _err;
      }
    }
  }

  for (u64 i = 0; i < pBone.getNumChildren(); ++i) {
    std::string _err;
    auto err = gatherBoneRecursive(output, pBone.getChild(i), view, v_mtx,
                                   p_mtx, render_data, _err, type);
    auto err2 = err.has_value() ? "" : err.error();
    if (_err.size()) {
      err2 = err2 + "\n" + _err;
    }
    if (err2.size()) {
      return std::unexpected(err2);
    }
  }

  return {};
}

std::string gather(lib3d::SceneBuffers& output, ModelView view, glm::mat4 v_mtx,
                   glm::mat4 p_mtx, G3dSceneRenderData& render_data,
                   lib3d::RenderType type = lib3d::RenderType::Preview) {
  if (view.mats.empty() || view.polys.empty() || view.bones.empty())
    return {};

  // Assumes root at zero
  std::string _err;
  auto err = gatherBoneRecursive(output, 0, view, v_mtx, p_mtx, render_data,
                                 _err, type);
  auto err2 = err.has_value() ? "" : err.error();
  if (_err.size()) {
    err2 = err2 + "\n" + _err;
  }
  return err2;
}

std::unique_ptr<G3dSceneRenderData>
G3DSceneCreateRenderData(riistudio::g3d::Collection& scene) {
  auto result = std::make_unique<G3dSceneRenderData>();
  result->init(scene);
  return result;
}

// This code is shared between J3D and G3D right now
Result<void> G3DSceneAddNodesToBuffer(riistudio::lib3d::SceneState& state,
                                      const riistudio::g3d::Collection& scene,
                                      glm::mat4 v_mtx, glm::mat4 p_mtx,
                                      G3dSceneRenderData& render_data) {
  // Reupload changed textures
  render_data.mTextureData.update(scene);

  state.getBuffers().opaque.nodes.reserve(256);
  state.getBuffers().translucent.nodes.reserve(256);
  std::string _err;
  int i = 0;
  for (auto& model : scene.getModels()) {
    ModelView view(model, scene);
    view.model_id = i++;
    auto err = gather(state.getBuffers(), view, v_mtx, p_mtx, render_data);
    if (err.size()) {
      _err = _err + "\n" + err;
    }
  }
  return std::unexpected(_err);
}

Result<void> Any3DSceneAddNodesToBuffer(riistudio::lib3d::SceneState& state,
                                        const libcube::Scene& scene,
                                        glm::mat4 v_mtx, glm::mat4 p_mtx,
                                        G3dSceneRenderData& render_data,
                                        lib3d::RenderType type) {
  // Reupload changed textures
  render_data.mTextureData.update(scene);
  TRY(render_data.mVertexRenderData.update(scene));

  int i = 0;
  for (auto& model : scene.getModels()) {
    ModelView view(model, scene);
    view.model_id = i++;
    auto err =
        gather(state.getBuffers(), view, v_mtx, p_mtx, render_data, type);
    if (err.size()) {
      return std::unexpected(err);
    }
  }
  return {};
}

} // namespace librii::g3d::gfx
