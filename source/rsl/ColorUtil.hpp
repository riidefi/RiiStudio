#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <math.h>

namespace rsl {

inline glm::vec3 hue_translate(glm::vec3 in, float H) {
  float U = cos(H * 3.141592f / 180.0f);
  float W = sin(H * 3.141592f / 180.0f);

  glm::vec3 ret;
  ret.r = (.299 + .701 * U + .168 * W) * in.r +
          (.587 - .587 * U + .330 * W) * in.g +
          (.114 - .114 * U - .497 * W) * in.b;
  ret.g = (.299 - .299 * U - .328 * W) * in.r +
          (.587 + .413 * U + .035 * W) * in.g +
          (.114 - .114 * U + .292 * W) * in.b;
  ret.b = (.299 - .3 * U + 1.25 * W) * in.r +
          (.587 - .588 * U - 1.05 * W) * in.g +
          (.114 + .886 * U - .203 * W) * in.b;
  return ret;
}

inline glm::vec4 color_from_hex(u32 hex) {
  return {
      static_cast<float>((hex & 0xFF'00'00'00) >> 24) / 255.0f,
      static_cast<float>((hex & 0x00'FF'00'00) >> 16) / 255.0f,
      static_cast<float>((hex & 0x00'00'FF'00) >> 8) / 255.0f,
      static_cast<float>((hex & 0x00'00'00'FF) >> 0) / 255.0f,
  };
}

} // namespace rsl
