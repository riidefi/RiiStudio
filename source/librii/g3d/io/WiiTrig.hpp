#pragma once

#include <core/common.h>
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>
#include <optional>

#include <librii/g3d/data/ModelData.hpp> // ScalingRule

namespace librii::g3d {

f32 WiiSin(f32 fidx);
f32 WiiCos(f32 fidx);

// Convert into 1/256ths of a circle
// TODO: Determine precision radian conversion factor
inline f64 DegreesToFIDX(f32 degrees) { return degrees * (256.0f / 360.0f); }

// COLUMN-MAJOR IMPLEMENTATION
glm::mat4x3 MTXConcat(const glm::mat4x3& a, const glm::mat4x3& b);

// COLUMN-MAJOR IMPLEMENTATION
void Mtx_makeRotateDegrees(glm::mat4x3& out, f64 rx, f64 ry, f64 rz);
inline void Mtx_makeRotateDegrees(glm::mat4& out, f64 rx, f64 ry, f64 rz) {
  glm::mat4x3 tmp;
  Mtx_makeRotateDegrees(tmp, rx, ry, rz);
  out = tmp;
}

void Mtx_scale(glm::mat4x3& out, const glm::mat4x3& mtx, const glm::vec3& scl);
std::optional<glm::mat4x3> MTXInverse(const glm::mat4x3& mtx);

using WiiFloat = f64;

// COLUMN-MAJOR IMPLEMENTATION
inline void CalcEnvelopeContribution(glm::mat4& thisMatrix,
                                     glm::vec3& thisScale, const auto& node,
                                     const glm::mat4& parentMtx,
                                     const glm::vec3& parentScale,
                                     librii::g3d::ScalingRule scalingRule) {
  auto srt = getSrt(node);
  if (ssc(node)) {
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

// SLOW IMPLEMENTATION
inline glm::mat4 calcSrtMtx(const auto& bone, auto&& bones,
                            librii::g3d::ScalingRule scalingRule) {
  std::vector<s32> path;
  auto* it = &bone;
  while (true) {
    s32 parentIndex = parentOf(*it);
    if (parentIndex < 0 || parentIndex >= std::ranges::size(bones)) {
      break;
    }
    path.push_back(parentIndex);
    it = &bones[parentIndex];
  }

  glm::mat4 mat(1.0f);
  glm::vec3 scl(1.0f, 1.0f, 1.0f);
  for (int i = 0; i < path.size(); ++i) {
    auto index = path[path.size() - 1 - i];
    auto& thisBone = bones[index];

    glm::mat4 thisMatrix(1.0f);
    glm::vec3 thisScale(1.0f, 1.0f, 1.0f);
    CalcEnvelopeContribution(/*out*/ thisMatrix, /*out*/ thisScale, thisBone,
                             mat, scl, scalingRule);
    mat = thisMatrix;
    scl = thisScale;
  }
  glm::mat4 thisMatrix(1.0f);
  glm::vec3 thisScale(1.0f, 1.0f, 1.0f);
  {
    CalcEnvelopeContribution(/*out*/ thisMatrix, /*out*/ thisScale, bone, mat,
                             scl, scalingRule);
  }
  // return glm::scale(thisMatrix, thisScale);
  glm::mat4x3 tmp;
  librii::g3d::Mtx_scale(tmp, thisMatrix, thisScale);
  return tmp;
}

} // namespace librii::g3d
