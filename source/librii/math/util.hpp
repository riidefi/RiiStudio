#pragma once

namespace librii::math {

constexpr static float map(float value, float minIn, float maxIn, float minOut,
                           float maxOut) {
  return (value - minIn) * (maxOut - minOut) / (maxIn - minIn) + minOut;
}

/// <summary>
/// hermite interpolation between 2 keyframes (left, right) with a frame, a value and a tangent each
/// </summary>
/// <returns>The interpolated value</returns>
constexpr static float hermite(float frame, float left_frame, float left_value,
                               float left_tangent, float right_frame,
                               float right_value, float right_tangent) {

  assert(left_frame <= frame && frame <= right_frame);
  float t = map(frame, left_frame, right_frame, 0, 1);

  float inv_t = t - 1.0f; //-1 to 0

  return left_value +
         (frame - left_frame) * inv_t *
             (inv_t * left_tangent + t * right_tangent) +
         t * t * (3.0f - 2.0f * t) * (right_value - left_value);
}

} // namespace librii::math
