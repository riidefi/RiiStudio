#pragma once

#include <math.h>
#include <vendor/glm/mat4x4.hpp>
#include <vendor/glm/trigonometric.hpp>
#include <vendor/glm/vec3.hpp>

namespace librii::math {

struct SRT3 {
  glm::vec3 scale;
  glm::vec3 rotation;
  glm::vec3 translation;

  bool operator==(const SRT3& rhs) const = default;
  bool operator!=(const SRT3& rhs) const { return !operator==(rhs); }
};

inline glm::mat4 calcXform(const SRT3& srt) {
  glm::mat4 dst(1.0f);

  float sinX = sin(glm::radians(srt.rotation.x)),
        cosX = cos(glm::radians(srt.rotation.x));
  float sinY = sin(glm::radians(srt.rotation.y)),
        cosY = cos(glm::radians(srt.rotation.y));
  float sinZ = sin(glm::radians(srt.rotation.z)),
        cosZ = cos(glm::radians(srt.rotation.z));

  dst[0][0] = srt.scale.x * (cosY * cosZ);
  dst[0][1] = srt.scale.x * (sinZ * cosY);
  dst[0][2] = srt.scale.x * (-sinY);
  dst[0][3] = 0.0;
  dst[1][0] = srt.scale.y * (sinX * cosZ * sinY - cosX * sinZ);
  dst[1][1] = srt.scale.y * (sinX * sinZ * sinY + cosX * cosZ);
  dst[1][2] = srt.scale.y * (sinX * cosY);
  dst[1][3] = 0.0;
  dst[2][0] = srt.scale.z * (cosX * cosZ * sinY + sinX * sinZ);
  dst[2][1] = srt.scale.z * (cosX * sinZ * sinY - sinX * cosZ);
  dst[2][2] = srt.scale.z * (cosY * cosX);
  dst[2][3] = 0.0;
  dst[3][0] = srt.translation.x;
  dst[3][1] = srt.translation.y;
  dst[3][2] = srt.translation.z;
  dst[3][3] = 1.0;

  return dst;
}

} // namespace librii::math
