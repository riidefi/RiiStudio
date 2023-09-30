#include "bindings.h"

#include "RsMeshOpt.hpp"

u32 c_stripify(u32* dst, u32 algo, const u32* indices, u32 num_indices,
               const float* positions, u32 num_positions, u32 restart) {
  std::span<const u32> index_data(indices, num_indices);
  std::span<const rsmeshopt::vec3> vertex_data(
      reinterpret_cast<const rsmeshopt::vec3*>(positions), num_positions);

  auto result =
      rsmeshopt::DoStripifyAlgo(static_cast<rsmeshopt::StripifyAlgo>(algo),
                                index_data, vertex_data, restart);

  if (result) {
    const auto& vec = result.value();
    std::copy(vec.begin(), vec.end(), dst);
    return vec.size();
  } else {
    return 0;
  }
}

// This would be how you make the C wrapper for the second function
u32 c_makefans(u32* dst, const u32* indices, u32 num_indices, u32 restart,
               u32 min_len, u32 max_runs) {
  std::span<const u32> index_data(indices, num_indices);

  auto result = rsmeshopt::MakeFans(index_data, restart, min_len, max_runs);

  if (result) {
    const auto& vec = result.value();
    std::copy(vec.begin(), vec.end(), dst);
    return vec.size();
  } else {
    return 0;
  }
}
