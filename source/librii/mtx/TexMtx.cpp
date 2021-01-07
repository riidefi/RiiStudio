#include "TexMtx.hpp"

namespace librii::mtx {

static void calcTexMtx_Basic(glm::mat4& dst, float scaleS, float scaleT,
                             float rotation, float translationS,
                             float translationT, float centerS, float centerT,
                             float centerQ) {
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
static void calcTexMtx_Maya(glm::mat4& dst, float scaleS, float scaleT,
                            float rotation, float translationS,
                            float translationT) {
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
static void computeNormalMatrix(glm::mat4& dst, const glm::mat4& m,
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
static void texEnvMtx(glm::mat4& dst, float scaleS, float scaleT, float transS,
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
static void buildEnvMtxOld(glm::mat4& dst, float flipYScale) {
  // Map from -1...1 range to 0...1 range.
  texEnvMtx(dst, 0.5, 0.5 * flipYScale, 0.5, 0.5);

  dst[2][2] = 1.0;
  dst[3][2] = 0.0;
}

static void buildEnvMtx(glm::mat4& dst, float flipYScale) {
  // Map from -1...1 range to 0...1 range.
  texEnvMtx(dst, 0.5, 0.5 * flipYScale, 0.5, 0.5);

  // texEnvMtx puts translation in fourth column, so we need to swap.
  std::swap(dst[3][0], dst[2][0]);
  std::swap(dst[3][1], dst[2][1]);
  std::swap(dst[3][2], dst[2][2]);
}

glm::mat4 computeTexSrt(const glm::vec2& scale, f32 rotate,
                        const glm::vec2& translate, bool maya) {
  glm::mat4 texsrt(1.0f);
  if (maya) {
    calcTexMtx_Maya(texsrt, scale.x, scale.y, rotate, translate.x, translate.y);
  } else {
    calcTexMtx_Basic(texsrt, scale.x, scale.y, rotate, translate.x, translate.y,
                     0.5, 0.5, 0.5);
  }
  return texsrt;
}

static glm::mat4 computeInMtx(const glm::mat4& mdl, const glm::mat4& mvp,
                              CommonMappingMethod method) {
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
    assert(!"Unsupported mapping method!");
    break;
  }
  return inmtx;
}

glm::mat4 computeTexMtx(const glm::mat4& mdl, const glm::mat4& mvp,
                        const glm::mat4& texsrt, CommonMappingMethod method,
                        CommonMappingOption option) {
  auto inmtx = computeInMtx(mdl, mvp, method);
  auto J3DGetTextureMtxOld = [](glm::mat4& dst, const glm::mat4& srt) {
    dst = srt;
  };
  auto J3DGetTextureMtx = [](glm::mat4& dst, const glm::mat4& srt) {
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
    default:
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
    default:
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
      tmp1 = mvp;
      tmp1 = tmp2 * tmp1;
      dst = tmp1 * dst;
      break;
    case CommonMappingMethod::ProjectionMapping:
      J3DGetTextureMtx(tmp2, texsrt);
      buildEnvMtx(tmp1, flipYScale);
      tmp2 = tmp2 * tmp1;

      // Multiply the effect matrix by the inverse of the model matrix.
      // In Galaxy, this is done in ProjmapEffectMtxSetter.
      // tmp1 = glm::inverse(mdl);
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
      break;
    }
  }

  return dst;
}

} // namespace librii::mtx
