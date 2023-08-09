#pragma once

#include <core/common.h>
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>
#include <optional>

#include <librii/g3d/data/ModelData.hpp> // ScalingRule
#include <librii/math/srt3.hpp>

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
void CalcEnvelopeContribution(glm::mat4& thisMatrix, glm::vec3& thisScale,
                              const librii::math::SRT3& srt, bool ssc,
                              const glm::mat4& parentMtx,
                              const glm::vec3& parentScale,
                              librii::g3d::ScalingRule scalingRule);

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
    CalcEnvelopeContribution(/*out*/ thisMatrix, /*out*/ thisScale,
                             getSrt(thisBone), ssc(thisBone), mat, scl,
                             scalingRule);
    mat = thisMatrix;
    scl = thisScale;
  }
  glm::mat4 thisMatrix(1.0f);
  glm::vec3 thisScale(1.0f, 1.0f, 1.0f);
  {
    CalcEnvelopeContribution(/*out*/ thisMatrix, /*out*/ thisScale,
                             getSrt(bone), ssc(bone), mat, scl, scalingRule);
  }
  // return glm::scale(thisMatrix, thisScale);
  glm::mat4x3 tmp;
  librii::g3d::Mtx_scale(tmp, thisMatrix, thisScale);
  return tmp;
}

} // namespace librii::g3d
