#pragma once

#include <rsl/Result.hpp>
#include <rsl/Types.hpp>

namespace rsl {

static inline Result<f32> CheckFloat(f32 in) {
  if (std::isinf(in)) {
    return in > 0.0f ? std::unexpected("Float is set to INFINITY")
                     : std::unexpected("Float is set to -INFINITY");
  }
  if (std::isnan(in)) {
    return in > 0.0f ? std::unexpected("Float is set to NAN")
                     : std::unexpected("Float is set to -NAN");
  }
  return in;
}

} // namespace rsl
