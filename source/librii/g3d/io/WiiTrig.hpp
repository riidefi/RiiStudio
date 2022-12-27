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

} // namespace librii::g3d
