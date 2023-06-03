#pragma once

namespace librii::math {

static float map(float value, float minIn, float maxIn, float minOut, float maxOut) {
  return (value - minIn) * (maxOut - minOut) / (maxIn - minIn) + minOut;
}

} // namespace librii::math
