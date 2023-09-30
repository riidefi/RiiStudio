#include "WiiTrig.hpp"

#include <core/common.h>

#include <wiitrig/include/wiitrig.h>

namespace librii::g3d {

using WiiFloat = f64;

f32 WiiSin(f32 x) { return wii_sin(x); }

f32 WiiCos(f32 x) { return wii_cos(x); }

// COLUMN-MAJOR IMPLEMENTATION
glm::mat4x3 MTXConcat(const glm::mat4x3& a, const glm::mat4x3& b) {
  glm::mat4x3 v3;
  // clang-format off
  v3[0][0] = ((WiiFloat)a[0][0] * (WiiFloat)b[0][0] + (WiiFloat)a[1][0] * (WiiFloat)b[0][1] + (WiiFloat)a[2][0] * (WiiFloat)b[0][2]);
  v3[1][0] = ((WiiFloat)a[0][0] * (WiiFloat)b[1][0] + (WiiFloat)a[1][0] * (WiiFloat)b[1][1] + (WiiFloat)a[2][0] * (WiiFloat)b[1][2]);
  v3[2][0] = ((WiiFloat)a[0][0] * (WiiFloat)b[2][0] + (WiiFloat)a[1][0] * (WiiFloat)b[2][1] + (WiiFloat)a[2][0] * (WiiFloat)b[2][2]);
  v3[3][0] = ((WiiFloat)a[0][0] * (WiiFloat)b[3][0] + (WiiFloat)a[1][0] * (WiiFloat)b[3][1] + (WiiFloat)a[2][0] * (WiiFloat)b[3][2] + (WiiFloat)a[3][0]);
  v3[0][1] = ((WiiFloat)a[0][1] * (WiiFloat)b[0][0] + (WiiFloat)a[1][1] * (WiiFloat)b[0][1] + (WiiFloat)a[2][1] * (WiiFloat)b[0][2]);
  v3[1][1] = ((WiiFloat)a[0][1] * (WiiFloat)b[1][0] + (WiiFloat)a[1][1] * (WiiFloat)b[1][1] + (WiiFloat)a[2][1] * (WiiFloat)b[1][2]);
  v3[2][1] = ((WiiFloat)a[0][1] * (WiiFloat)b[2][0] + (WiiFloat)a[1][1] * (WiiFloat)b[2][1] + (WiiFloat)a[2][1] * (WiiFloat)b[2][2]);
  v3[3][1] = ((WiiFloat)a[0][1] * (WiiFloat)b[3][0] + (WiiFloat)a[1][1] * (WiiFloat)b[3][1] + (WiiFloat)a[2][1] * (WiiFloat)b[3][2] + (WiiFloat)a[3][1]);
  v3[0][2] = ((WiiFloat)a[0][2] * (WiiFloat)b[0][0] + (WiiFloat)a[1][2] * (WiiFloat)b[0][1] + (WiiFloat)a[2][2] * (WiiFloat)b[0][2]);
  v3[1][2] = ((WiiFloat)a[0][2] * (WiiFloat)b[1][0] + (WiiFloat)a[1][2] * (WiiFloat)b[1][1] + (WiiFloat)a[2][2] * (WiiFloat)b[1][2]);
  v3[2][2] = ((WiiFloat)a[0][2] * (WiiFloat)b[2][0] + (WiiFloat)a[1][2] * (WiiFloat)b[2][1] + (WiiFloat)a[2][2] * (WiiFloat)b[2][2]);
  v3[3][2] = ((WiiFloat)a[0][2] * (WiiFloat)b[3][0] + (WiiFloat)a[1][2] * (WiiFloat)b[3][1] + (WiiFloat)a[2][2] * (WiiFloat)b[3][2] + (WiiFloat)a[3][2]);
  // clang-format on
  return v3;
}

// COLUMN-MAJOR IMPLEMENTATION
void Mtx_makeRotateFIdx(glm::mat4x3& mtx, f64 rx, f64 ry, f64 rz) {
  f32 sinxf32 = WiiSin(rx);
  f32 cosxf32 = WiiCos(rx);
  f32 sinyf32 = WiiSin(ry);
  f32 cosyf32 = WiiCos(ry);
  f32 sinzf32 = WiiSin(rz);
  f32 coszf32 = WiiCos(rz);
  WiiFloat sinx = static_cast<WiiFloat>(sinxf32);
  WiiFloat cosx = static_cast<WiiFloat>(cosxf32);
  WiiFloat siny = static_cast<WiiFloat>(sinyf32);
  WiiFloat cosy = static_cast<WiiFloat>(cosyf32);
  WiiFloat sinz = static_cast<WiiFloat>(sinzf32);
  WiiFloat cosz = static_cast<WiiFloat>(coszf32);
  mtx[0][0] = cosz * cosy;
  mtx[0][1] = sinz * cosy;
  mtx[0][2] = -siny;
  mtx[1][0] = (sinx * cosz) * siny - (cosx * sinz);
  mtx[1][1] = (sinx * sinz) * siny + (cosx * cosz);
  mtx[1][2] = cosy * sinx;
  mtx[2][0] = (cosx * cosz) * siny + (sinx * sinz);
  mtx[2][1] = (cosx * sinz) * siny - (sinx * cosz);
  mtx[2][2] = cosy * cosx;
  mtx[3][0] = 0.0f;
  mtx[3][1] = 0.0f;
  mtx[3][2] = 0.0f;
}

void Mtx_makeRotateDegrees(glm::mat4x3& out, f64 rx, f64 ry, f64 rz) {
  Mtx_makeRotateFIdx(out, DegreesToFIDX(rx), DegreesToFIDX(ry),
                     DegreesToFIDX(rz));
}

void Mtx_scale(glm::mat4x3& out, const glm::mat4x3& mtx, const glm::vec3& scl) {
  out[0][0] = (WiiFloat)mtx[0][0] * (WiiFloat)scl.x;
  out[0][1] = (WiiFloat)mtx[0][1] * (WiiFloat)scl.x;
  out[0][2] = (WiiFloat)mtx[0][2] * (WiiFloat)scl.x;
  out[1][0] = (WiiFloat)mtx[1][0] * (WiiFloat)scl.y;
  out[1][1] = (WiiFloat)mtx[1][1] * (WiiFloat)scl.y;
  out[1][2] = (WiiFloat)mtx[1][2] * (WiiFloat)scl.y;
  out[2][0] = (WiiFloat)mtx[2][0] * (WiiFloat)scl.z;
  out[2][1] = (WiiFloat)mtx[2][1] * (WiiFloat)scl.z;
  out[2][2] = (WiiFloat)mtx[2][2] * (WiiFloat)scl.z;
  if (&out != &mtx) {
    out[3][0] = mtx[3][0];
    out[3][1] = mtx[3][1];
    out[3][2] = mtx[3][2];
  }
}

// COLUMN-MAJOR IMPLEMENTATION
std::optional<glm::mat4x3> MTXInverse(const glm::mat4x3& mtx) {
  glm::mat4x3 out;
  f32 determinant =
      (WiiFloat)mtx[0][0] * (WiiFloat)mtx[1][1] * (WiiFloat)mtx[2][2] +
      (WiiFloat)mtx[1][0] * (WiiFloat)mtx[2][1] * (WiiFloat)mtx[0][2] +
      (WiiFloat)mtx[2][0] * (WiiFloat)mtx[0][1] * (WiiFloat)mtx[1][2] -
      (WiiFloat)mtx[0][2] * (WiiFloat)mtx[1][1] * (WiiFloat)mtx[2][0] -
      (WiiFloat)mtx[0][1] * (WiiFloat)mtx[1][0] * (WiiFloat)mtx[2][2] -
      (WiiFloat)mtx[0][0] * (WiiFloat)mtx[1][2] * (WiiFloat)mtx[2][1];
  if (determinant == 0.0f) {
    return std::nullopt;
  }
  f32 coeff = 1.0f / determinant;
  // clang-format off

  out[0][0] =  ((WiiFloat)mtx[1][1] * (WiiFloat)mtx[2][2] - (WiiFloat)mtx[1][2] * (WiiFloat)mtx[2][1]) * coeff;
#ifdef __APPLE__
  {
    WiiFloat a = (WiiFloat)mtx[1][0] * (WiiFloat)mtx[2][2];
    WiiFloat b = (WiiFloat)mtx[1][2] * (WiiFloat)mtx[2][0];
    WiiFloat c = a-b;
    WiiFloat d = -c;
    WiiFloat e = d * coeff;
    out[1][0] = e;
  }
#else
  out[1][0] = -((WiiFloat)mtx[1][0] * (WiiFloat)mtx[2][2] - (WiiFloat)mtx[1][2] * (WiiFloat)mtx[2][0]) * coeff;
#endif
  out[2][0] =  ((WiiFloat)mtx[1][0] * (WiiFloat)mtx[2][1] - (WiiFloat)mtx[1][1] * (WiiFloat)mtx[2][0]) * coeff;
#ifdef __APPLE__
  {
    WiiFloat a = (WiiFloat)mtx[0][1] * (WiiFloat)mtx[2][2];
    WiiFloat b = (WiiFloat)mtx[0][2] * (WiiFloat)mtx[2][1];
    WiiFloat c = a-b;
    WiiFloat d = -c;
    WiiFloat e = d * coeff;
    out[0][1] = e;
  }
#else
  out[0][1] = -((WiiFloat)mtx[0][1] * (WiiFloat)mtx[2][2] - (WiiFloat)mtx[0][2] * (WiiFloat)mtx[2][1]) * coeff;
#endif
  out[1][1] =  ((WiiFloat)mtx[0][0] * (WiiFloat)mtx[2][2] - (WiiFloat)mtx[0][2] * (WiiFloat)mtx[2][0]) * coeff;
#ifdef __APPLE__
  {
    WiiFloat a = (WiiFloat)mtx[0][0] * (WiiFloat)mtx[2][1];
    WiiFloat b = (WiiFloat)mtx[0][1] * (WiiFloat)mtx[2][0];
    WiiFloat c = a-b;
    WiiFloat d = -c;
    WiiFloat e = d * coeff;
    out[2][1] = e;
  }
#else
  out[2][1] = -((WiiFloat)mtx[0][0] * (WiiFloat)mtx[2][1] - (WiiFloat)mtx[0][1] * (WiiFloat)mtx[2][0]) * coeff;
#endif
  out[0][2] =  ((WiiFloat)mtx[0][1] * (WiiFloat)mtx[1][2] - (WiiFloat)mtx[0][2] * (WiiFloat)mtx[1][1]) * coeff;
#ifdef __APPLE__
  {
    WiiFloat a = (WiiFloat)mtx[0][0] * (WiiFloat)mtx[1][2];
    WiiFloat b = (WiiFloat)mtx[0][2] * (WiiFloat)mtx[1][0];
    WiiFloat c = a-b;
    WiiFloat d = -c;
    WiiFloat e = d * coeff;
    out[1][2] = e;
  }
#else
  out[1][2] = -((WiiFloat)mtx[0][0] * (WiiFloat)mtx[1][2] - (WiiFloat)mtx[0][2] * (WiiFloat)mtx[1][0]) * coeff;
#endif
  out[2][2] =  ((WiiFloat)mtx[0][0] * (WiiFloat)mtx[1][1] - (WiiFloat)mtx[0][1] * (WiiFloat)mtx[1][0]) * coeff;
  out[3][0] = -out[0][0] * (WiiFloat)mtx[3][0] - (WiiFloat)out[1][0] * (WiiFloat)mtx[3][1] - (WiiFloat)out[2][0] * (WiiFloat)mtx[3][2];
  out[3][1] = -out[0][1] * (WiiFloat)mtx[3][0] - (WiiFloat)out[1][1] * (WiiFloat)mtx[3][1] - (WiiFloat)out[2][1] * (WiiFloat)mtx[3][2];
  out[3][2] = -out[0][2] * (WiiFloat)mtx[3][0] - (WiiFloat)out[1][2] * (WiiFloat)mtx[3][1] - (WiiFloat)out[2][2] * (WiiFloat)mtx[3][2];
  // clang-format on
  return out;
}

// COLUMN-MAJOR IMPLEMENTATION
void CalcEnvelopeContribution(glm::mat4& thisMatrix, glm::vec3& thisScale,
                              const librii::math::SRT3& srt, bool ssc,
                              const glm::mat4& parentMtx,
                              const glm::vec3& parentScale,
                              librii::g3d::ScalingRule scalingRule) {
  if (ssc) {
    librii::g3d::Mtx_makeRotateDegrees(/*out*/ thisMatrix, srt.rotation.x,
                                       srt.rotation.y, srt.rotation.z);
    thisMatrix[3][0] = (WiiFloat)srt.translation.x * (WiiFloat)parentScale.x;
    thisMatrix[3][1] = (WiiFloat)srt.translation.y * (WiiFloat)parentScale.y;
    thisMatrix[3][2] = (WiiFloat)srt.translation.z * (WiiFloat)parentScale.z;
    // PSMTXConcat(parentMtx, &thisMatrix, /*out*/ &thisMatrix);
    // thisMatrix = parentMtx * thisMatrix;
    thisMatrix = librii::g3d::MTXConcat(parentMtx, thisMatrix);
    thisScale.x = srt.scale.x;
    thisScale.y = srt.scale.y;
    thisScale.z = srt.scale.z;
  } else if (scalingRule ==
             librii::g3d::ScalingRule::XSI) { // CLASSIC_SCALE_OFF
    librii::g3d::Mtx_makeRotateDegrees(/*out*/ thisMatrix, srt.rotation.x,
                                       srt.rotation.y, srt.rotation.z);
    thisMatrix[3][0] = (WiiFloat)srt.translation.x * (WiiFloat)parentScale.x;
    thisMatrix[3][1] = (WiiFloat)srt.translation.y * (WiiFloat)parentScale.y;
    thisMatrix[3][2] = (WiiFloat)srt.translation.z * (WiiFloat)parentScale.z;
    // PSMTXConcat(parentMtx, &thisMatrix, /*out*/ &thisMatrix);
    // thisMatrix = parentMtx * thisMatrix;
    thisMatrix = librii::g3d::MTXConcat(parentMtx, thisMatrix);
    thisScale.x = (WiiFloat)srt.scale.x * (WiiFloat)parentScale.x;
    thisScale.y = (WiiFloat)srt.scale.y * (WiiFloat)parentScale.y;
    thisScale.z = (WiiFloat)srt.scale.z * (WiiFloat)parentScale.z;
  } else {
    glm::mat4x3 scratch;
    // Mtx_scale(/*out*/ &scratch, parentMtx, parentScale);
    librii::g3d::Mtx_scale(/*out*/ scratch, parentMtx, parentScale);
    // scratch = glm::scale(parentMtx, parentScale);
    librii::g3d::Mtx_makeRotateDegrees(/*out*/ thisMatrix, srt.rotation.x,
                                       srt.rotation.y, srt.rotation.z);
    thisMatrix[3][0] = srt.translation.x;
    thisMatrix[3][1] = srt.translation.y;
    thisMatrix[3][2] = srt.translation.z;
    // PSMTXConcat(scratch, &thisMatrix, /*out*/ &thisMatrix);
    // thisMatrix = scratch * thisMatrix;
    thisMatrix = librii::g3d::MTXConcat(scratch, thisMatrix);
    thisScale.x = srt.scale.x;
    thisScale.y = srt.scale.y;
    thisScale.z = srt.scale.z;
  }
}

} // namespace librii::g3d
