#include <algorithm>
#include <core/3d/gl.hpp>
#include <librii/gl/Compiler.hpp>
#include <librii/gl/EnumConverter.hpp>
#include <librii/glhelper/UBOBuilder.hpp>
#include <librii/mtx/TexMtx.hpp>
#include <plugins/gc/Export/IndexedPolygon.hpp>
#include <plugins/gc/Export/Material.hpp>

namespace libcube {

std::pair<std::string, std::string> IGCMaterial::generateShaders() const {
  auto result = librii::gl::compileShader(getMaterialData(), getName());

  assert(result);
  if (!result) {
    return {"Invalid", "Invalid"};
  }

  if (!applyCacheAgain)
    cachedPixelShader = result->fragment + "\n\n // End of shader";
  return {result->vertex, result->fragment};
}

glm::mat4x4 GCMaterialData::TexMatrix::compute(const glm::mat4& mdl,
                                               const glm::mat4& mvp) const {
  auto texsrt =
      librii::mtx::computeTexSrt(scale, rotate, translate, transformModel);
  return librii::mtx::computeTexMtx(mdl, mvp, texsrt, method, option);
}
void IGCMaterial::generateUniforms(
    librii::glhelper::DelegatedUBOBuilder& builder, const glm::mat4& M,
    const glm::mat4& V, const glm::mat4& P, u32 shaderId,
    const std::map<std::string, u32>& texIdMap,
    const riistudio::lib3d::Polygon& poly,
    const riistudio::lib3d::Scene& scn) const {

  // WebGL doesn't support binding=n in the shader
#ifdef __EMSCRIPTEN__
  glUniformBlockBinding(shaderId,
                        glGetUniformBlockIndex(shaderId, "ub_SceneParams"), 0);
  glUniformBlockBinding(
      shaderId, glGetUniformBlockIndex(shaderId, "ub_MaterialParams"), 1);
  glUniformBlockBinding(shaderId,
                        glGetUniformBlockIndex(shaderId, "ub_PacketParams"), 2);
#endif // __EMSCRIPTEN__

  int min;
  glGetActiveUniformBlockiv(shaderId, 0, GL_UNIFORM_BLOCK_DATA_SIZE, &min);
  // printf("Min block size: %i\n", min);
  builder.setBlockMin(0, min);
  glGetActiveUniformBlockiv(shaderId, 1, GL_UNIFORM_BLOCK_DATA_SIZE, &min);
  // printf("Min block size: %i\n", min);
  builder.setBlockMin(1, min);
  glGetActiveUniformBlockiv(shaderId, 2, GL_UNIFORM_BLOCK_DATA_SIZE, &min);
  // printf("Min block size: %i\n", min);
  builder.setBlockMin(2, min);

  librii::gl::UniformSceneParams scene;
  scene.projection = M * V * P;
  scene.Misc0 = {};

  const auto& data = getMaterialData();

  librii::gl::UniformMaterialParams tmp{};
  librii::gl::setUniformsFromMaterial(tmp, data);

  for (int i = 0; i < data.texMatrices.size(); ++i) {
    tmp.TexMtx[i] = glm::transpose(data.texMatrices[i].compute(M, V * P));
  }
  for (int i = 0; i < data.samplers.size(); ++i) {
    if (data.samplers[i].mTexture.empty())
      continue;
    const auto* texData =
        getTexture(reinterpret_cast<const libcube::Scene&>(scn),
                   data.samplers[i].mTexture);
    if (texData == nullptr)
      continue;
    tmp.TexParams[i] = glm::vec4{texData->getWidth(), texData->getHeight(), 0,
                                 data.samplers[i].mLodBias};
  }

  builder.tpush(0, scene);
  builder.tpush(1, tmp);
  // builder.tpush(2, pack);
  // ^^ filled in onSplice

  const s32 samplerIds[] = {0, 1, 2, 3, 4, 5, 6, 7};

  glUseProgram(shaderId);
  u32 uTexLoc = glGetUniformLocation(shaderId, "u_Texture");
  glUniform1iv(uTexLoc, 8, samplerIds);
}

void IGCMaterial::onSplice(librii::glhelper::DelegatedUBOBuilder& builder,
                           const riistudio::lib3d::Model& model,
                           const riistudio::lib3d::Polygon& poly,
                           u32 mpid) const {
  // builder.reset(2);
  librii::gl::PacketParams pack{};
  for (auto& p : pack.posMtx)
    p = glm::transpose(glm::mat4{1.0f});

  assert(dynamic_cast<const IndexedPolygon*>(&poly) != nullptr);
  const auto& ipoly = reinterpret_cast<const IndexedPolygon&>(poly);

  const auto mtx =
      ipoly.getPosMtx(reinterpret_cast<const libcube::Model&>(model), mpid);
  for (int p = 0; p < std::min(static_cast<std::size_t>(10), mtx.size()); ++p) {
    pack.posMtx[p] = glm::transpose(mtx[p]);
  }

  builder.tpush(2, pack);
}
void IGCMaterial::setMegaState(librii::gfx::MegaState& state) const {
  librii::gl::translateGfxMegaState(state, getMaterialData());
}
} // namespace libcube
