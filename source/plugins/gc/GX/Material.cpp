#include <core/3d/gl.hpp>

#include "Material.hpp"

#include <plugins/gc/GX/Shader/GXProgram.hpp>

namespace libcube {

std::pair<std::string, std::string> IGCMaterial::generateShaders() const {
  GXProgram program(GXMaterial{0, getName(), *const_cast<IGCMaterial *>(this)});
  return program.generateShaders();
}

/*
Layout in memory:
(Binding 0) Scene
(Binding 1) Mat
(Binding 2) Shape

<---
Scene
Mat
<---
Mat
Mat
Shape
<---
Shape
Shape

*/

struct UniformSceneParams {
  glm::mat4 projection;
  glm::vec4 Misc0;
};
// ROW_MAJOR
struct PacketParams {
  glm::mat3x4 posMtx[10];
};
template <typename T> inline glm::vec4 colorConvert(T clr) {
  const auto f32c = (gx::ColorF32)clr;
  return {f32c.r, f32c.g, f32c.b, f32c.a};
}

void calcTexMtx_Basic(glm::mat4 &dst, float scaleS, float scaleT,
                      float rotation, float translationS, float translationT,
                      float centerS, float centerT, float centerQ) {
  const auto theta = rotation * 3.141592f;
  const auto sinR = sin(theta);
  const auto cosR = cos(theta);

  dst = glm::mat4(1.0f);

  dst[0][0] = scaleS * cosR;
  dst[1][0] = scaleS * -sinR;
  dst[2][0] =
      translationS + centerS + scaleS * (sinR * centerT - cosR * centerS);

  dst[0][1] = scaleT * sinR;
  dst[1][1] = scaleT * cosR;
  dst[2][1] =
      translationT + centerT + -scaleT * (-sinR * centerS + cosR * centerT);
}

void calcTexMtx_Maya(glm::mat4 &dst, float scaleS, float scaleT, float rotation,
                     float translationS, float translationT) {
  const auto theta = rotation * 3.141592f;
  const auto sinR = sin(theta);
  const auto cosR = cos(theta);

  dst = glm::mat4(1.0f);

  dst[0][0] = scaleS * cosR;
  dst[1][0] = scaleT * -sinR;
  dst[2][0] = scaleS * ((-0.5 * cosR) - (0.5 * sinR - 0.5) - translationS);

  dst[0][1] = scaleS * sinR;
  dst[1][1] = scaleT * cosR;
  dst[2][1] = scaleT * ((-0.5 * cosR) + (0.5 * sinR - 0.5) + translationT) + 1;
}
void computeNormalMatrix(glm::mat4 &dst, const glm::mat4 &m,
                         bool isUniformScale) {
  if (dst != m)
    dst = m;

  dst[3][0] = 0;
  dst[3][1] = 0;
  dst[3][2] = 0;

  if (!isUniformScale) {
    dst = glm::inverse(dst);
    dst = glm::transpose(dst);
  }
}

void texEnvMtx(glm::mat4 &dst, float scaleS, float scaleT, float transS,
               float transT) {
  dst[0][0] = scaleS;
  dst[1][0] = 0.0;
  dst[2][0] = 0.0;
  dst[3][0] = transS;

  dst[0][1] = 0.0;
  dst[1][1] = -scaleT;
  dst[2][1] = 0.0;
  dst[3][1] = transT;

  dst[0][2] = 0.0;
  dst[1][2] = 0.0;
  dst[2][2] = 0.0;
  dst[3][2] = 1.0;

  dst[0][3] = 9999.0;
  dst[1][3] = 9999.0;
  dst[2][3] = 9999.0;
  dst[3][3] = 9999.0;
}

void buildEnvMtxOld(glm::mat4 &dst, float flipYScale) {
  // Map from -1...1 range to 0...1 range.
  texEnvMtx(dst, 0.5, 0.5 * flipYScale, 0.5, 0.5);

  dst[2][2] = 1.0;
  dst[3][2] = 0.0;
}
void buildEnvMtx(glm::mat4 &dst, float flipYScale) {
  // Map from -1...1 range to 0...1 range.
  texEnvMtx(dst, 0.5, 0.5 * flipYScale, 0.5, 0.5);

  // texEnvMtx puts translation in fourth column, so we need to swap.
  std::swap(dst[3][0], dst[2][0]);
  std::swap(dst[3][1], dst[2][1]);
  std::swap(dst[3][2], dst[2][2]);
}

glm::mat3x4 GCMaterialData::TexMatrix::compute(const glm::mat4 &mdl,
                                               const glm::mat4 &mvp) {
  assert(transformModel != CommonTransformModel::Max &&
         transformModel != CommonTransformModel::XSI);

  glm::mat4 texsrt(1.0f);
  if (transformModel == CommonTransformModel::Maya) {
    calcTexMtx_Maya(texsrt, scale.x, scale.y, rotate, translate.x, translate.y);
  } else {
    calcTexMtx_Basic(texsrt, scale.x, scale.y, rotate, translate.x, translate.y,
                     0.5, 0.5, 0.5);
  }
  glm::mat4 inmtx(1.0f);
  switch (method) {
  case CommonMappingMethod::Standard:
    break;
  case CommonMappingMethod::EnvironmentMapping:
    // MVP Norrmal matrix
    computeNormalMatrix(inmtx, mvp, true);
    break;
  case CommonMappingMethod::ProjectionMapping:
    inmtx = mdl;
    // Model matrix
    break;
  case CommonMappingMethod::ViewProjectionMapping:
    // MVP matrix
    inmtx = mvp;
    break;
  // J3D 5
  case CommonMappingMethod::ManualEnvironmentMapping:
    // Model normal matrix
    computeNormalMatrix(inmtx, mdl, true);
    break;
  default:
    throw "Unsupported mapping method!";
    break;
  }

  auto J3DGetTextureMtxOld = [](glm::mat4 &dst, const glm::mat4 &srt) {
    dst = srt;
  };
  auto J3DGetTextureMtx = [](glm::mat4 &dst, const glm::mat4 &srt) {
    dst = srt;

    // Move translation to third column.
    dst[2][0] = dst[3][0];
    dst[2][1] = dst[3][1];
    dst[2][2] = 1.0;

    dst[3][0] = 0;
    dst[3][1] = 0;
    dst[3][2] = 0;
  };
  glm::mat4 dst = inmtx;

  glm::mat4 tmp1(1.0f);
  glm::mat4 tmp2(1.0f);

  float flipYScale = 1.0f;

  if (option == CommonMappingOption::DontRemapTextureSpace) {
    switch (method) {
    case CommonMappingMethod::EnvironmentMapping:
      // J3DGetTextureMtxOld(tmp1)
      J3DGetTextureMtxOld(tmp1, texsrt);

      // PSMTXConcat(tmp1, inputMatrix, this->finalMatrix)
      dst = tmp1 * dst;
      break;
    case CommonMappingMethod::ProjectionMapping:
    case CommonMappingMethod::ViewProjectionMapping:
      // J3DGetTextureMtxOld(tmp2)
      J3DGetTextureMtxOld(tmp2, texsrt);

      //.. mtxFlipY(dst, flipY);

      // J3DMtxProjConcat(tmp2, this->effectMtx, tmp1)
      tmp1 = tmp2; //... * glm::mat4(effectMatrix);
      // PSMTXConcat(tmp1, inputMatrix, this->finalMatrix)
      dst = tmp1 * dst;
      break;
    }
  } else if (option == CommonMappingOption::KeepTranslation) {
    switch (method) {
    // J3D 04
    case CommonMappingMethod::EnvironmentMapping:
      J3DGetTextureMtxOld(tmp1, texsrt);
      buildEnvMtxOld(tmp2, flipYScale);
      dst = tmp1 * tmp2 * dst;
      break;
    case CommonMappingMethod::ManualEnvironmentMapping:
      J3DGetTextureMtxOld(tmp2, texsrt);
      buildEnvMtxOld(tmp1, flipYScale);
      tmp2 *= tmp1;

      tmp1 = tmp2; // *effectMatrix;

      // PSMTXConcat(tmp1, inputMatrix, this->finalMatrix)
      dst = tmp1 * dst;
      break;
    }
  } else {
    switch (method) {
    case CommonMappingMethod::EnvironmentMapping:
      J3DGetTextureMtx(tmp1, texsrt);
      buildEnvMtx(tmp2, flipYScale);
      tmp1 = tmp1 * tmp2;
      dst = tmp1 * dst;
      break;
    case CommonMappingMethod::ViewProjectionMapping:
      J3DGetTextureMtx(tmp2, texsrt);

      // The effect matrix here is a GameCube projection matrix. Swap it out
      // with out own. In Galaxy, this is done in ViewProjmapEffectMtxSetter.
      // Replaces the effectMatrix. EnvMtx is built into this call, as well.
      //... texProjCameraSceneTex(tmp1, camera, viewport, flipYScale);

      tmp1 = tmp2 * tmp1;
      dst = tmp1 * dst;
      break;
    case CommonMappingMethod::ProjectionMapping:
      J3DGetTextureMtx(tmp2, texsrt);
      buildEnvMtx(tmp1, flipYScale);
      tmp2 = tmp2 * tmp1;

      // Multiply the effect matrix by the inverse of the model matrix.
      // In Galaxy, this is done in ProjmapEffectMtxSetter.
      tmp1 = glm::inverse(mdl);
      //... tmp1 = effectMatrix * tmp1;

      tmp1 = tmp2 * tmp1;
      dst = tmp1 * dst;
      break;
    case CommonMappingMethod::ManualEnvironmentMapping:
      J3DGetTextureMtx(tmp2, texsrt);
      buildEnvMtx(tmp1, flipYScale);
      tmp2 = tmp2 * tmp1;

      // J3DMtxProjConcat(tmp2, this->effectMtx, tmp1)
      //.. tmp1 = tmp2 * effectMatrix;

      // PSMTXConcat(tmp1, inputMatrix, this->finalMatrix)
      dst = tmp1 * dst;
      break;
    default:
      // J3DGetTextureMtxOld(this->finalMatrix)
      J3DGetTextureMtxOld(dst, texsrt);

      //... mtxFlipY(dst, flipY);
    }
  }

  return dst;
}
void IGCMaterial::generateUniforms(
    DelegatedUBOBuilder &builder, const glm::mat4 &M, const glm::mat4 &V,
    const glm::mat4 &P, u32 shaderId,
    const std::map<std::string, u32> &texIdMap) const {
  glUniformBlockBinding(shaderId,
                        glGetUniformBlockIndex(shaderId, "ub_SceneParams"), 0);
  glUniformBlockBinding(
      shaderId, glGetUniformBlockIndex(shaderId, "ub_MaterialParams"), 1);
  glUniformBlockBinding(shaderId,
                        glGetUniformBlockIndex(shaderId, "ub_PacketParams"), 2);

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

  UniformSceneParams scene;
  scene.projection = M * V * P;
  scene.Misc0 = {};

  UniformMaterialParams tmp{};

  const auto &data = getMaterialData();

  for (int i = 0; i < 2; ++i) {
    // TODO: Broken
    tmp.ColorMatRegs[i] =
        glm::vec4{1.0f}; // colorConvert(data.chanData[i].matColor);
    tmp.ColorAmbRegs[i] =
        glm::vec4{1.0f}; // colorConvert(data.chanData[i].ambColor);
  }
  for (int i = 0; i < 4; ++i) {
    tmp.KonstColor[i] = colorConvert(data.tevKonstColors[i]);
    tmp.Color[i] = colorConvert(data.tevColors[i]);
  }
  for (int i = 0; i < data.texMatrices.size(); ++i) {
    tmp.TexMtx[i] = glm::transpose(data.texMatrices[i]->compute(M, M * V * P));
  }
  for (int i = 0; i < data.samplers.size(); ++i) {
    const auto &texData = getTexture(data.samplers[i]->mTexture);

    tmp.TexParams[i] = glm::vec4{texData.getWidth(), texData.getHeight(), 0,
                                 data.samplers[i]->mLodBias};
  }
  for (int i = 0; i < data.mIndMatrices.size(); ++i) {
    tmp.IndTexMtx[i] = data.mIndMatrices[i].compute();
  }
  PacketParams pack{};
  for (auto &p : pack.posMtx)
    p = glm::transpose(glm::mat4{1.0f});

  builder.tpush(0, scene);
  builder.tpush(1, tmp);
  builder.tpush(2, pack);

  const s32 samplerIds[] = {0, 1, 2, 3, 4, 5, 6, 7};

  glUseProgram(shaderId);
  u32 uTexLoc = glGetUniformLocation(shaderId, "u_Texture");
  glUniform1iv(uTexLoc, 8, samplerIds);
}

void IGCMaterial::genSamplUniforms(
    u32 shaderId, const std::map<std::string, u32> &texIdMap) const {
  const auto &data = getMaterialData();
  for (int i = 0; i < data.samplers.size(); ++i) {
    glActiveTexture(GL_TEXTURE0 + i);
    if (texIdMap.find(data.samplers[i]->mTexture) == texIdMap.end())
      printf("Invalid texture link.\n");
    // else printf("Tex id: %u\n", texIdMap.at(data.samplers[i]->mTexture));
    glBindTexture(GL_TEXTURE_2D, texIdMap.at(data.samplers[i]->mTexture));

    auto gxFilterToGl = [](gx::TextureFilter filter) {
      switch (filter) {
      case gx::TextureFilter::linear:
        return GL_LINEAR;
      case gx::TextureFilter::near:
        return GL_NEAREST;
      case gx::TextureFilter::lin_mip_lin:
        return GL_LINEAR_MIPMAP_LINEAR;
      case gx::TextureFilter::lin_mip_near:
        return GL_LINEAR_MIPMAP_NEAREST;
      case gx::TextureFilter::near_mip_lin:
        return GL_NEAREST_MIPMAP_LINEAR;
      case gx::TextureFilter::near_mip_near:
        return GL_NEAREST_MIPMAP_NEAREST;
      }
    };
    auto gxTileToGl = [](gx::TextureWrapMode wrap) {
      switch (wrap) {
      case gx::TextureWrapMode::Clamp:
        return GL_CLAMP_TO_EDGE;
      case gx::TextureWrapMode::Repeat:
        return GL_REPEAT;
      case gx::TextureWrapMode::Mirror:
        return GL_MIRRORED_REPEAT;
      }
    };

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    gxFilterToGl(data.samplers[i]->mMinFilter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    gxFilterToGl(data.samplers[i]->mMagFilter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    gxTileToGl(data.samplers[i]->mWrapU));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                    gxTileToGl(data.samplers[i]->mWrapV));
  }
}
void IGCMaterial::setMegaState(MegaState &state) const {
  GXMaterial mat{0, getName(), *const_cast<IGCMaterial *>(this)};

  translateGfxMegaState(state, mat);
}
} // namespace libcube
