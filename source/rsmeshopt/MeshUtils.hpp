#pragma once

#include <core/common.h>
#include <coro/generator.hpp>
#include <ranges>

namespace rsmeshopt::MeshUtils {

// Splits a list of indices |mesh| by a delimeter |primitive_restart_index|.
// https://www.khronos.org/opengl/wiki/Vertex_Rendering#Primitive_Restart
template <typename IndexTypeT>
static coro::generator<std::span<const IndexTypeT>>
SplitByPrimitiveRestart(std::span<const IndexTypeT> mesh,
                        IndexTypeT primitive_restart_index) {
  size_t last = 0;
  for (size_t i = 0; i <= mesh.size(); ++i) {
    if (i == mesh.size() || mesh[i] == primitive_restart_index) {
      auto subspan = mesh.subspan(last, i - last);
      if (!subspan.empty()) {
        co_yield subspan;
      }
      last = i + 1;
      continue;
    }
  }
}

// Determines if a list of triangle indices |mesh| contains duplicates
template <typename IndexTypeT>
static bool TriangleArrayHoldsDuplicates(std::span<const IndexTypeT> mesh) {
  std::vector<std::array<IndexTypeT, 3>> cache;
  for (size_t i = 0; i < mesh.size(); i += 3) {
    std::array<IndexTypeT, 3> cand{mesh[i], mesh[i + 1], mesh[i + 2]};
    auto it = std::ranges::find(cache, cand);
    if (it == cache.end()) {
      cache.push_back(cand);
      continue;
    }
    return true;
  }
  return false;
}

} // namespace rsmeshopt::MeshUtils
