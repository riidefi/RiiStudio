#pragma once

#include <core/common.h>
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>

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

} // namespace librii::g3d
