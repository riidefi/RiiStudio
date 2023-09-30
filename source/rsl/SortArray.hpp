#pragma once

#include <algorithm>
#include <type_traits>
#include <vector>

namespace rsl {

// Keys can be crazy, even, like -30 1 5 100
[[nodiscard]] static inline auto SortArrayByKeys(auto& arr,
                                                 const auto& sortIndices) {
  std::vector<size_t> indices(std::size(arr));
  std::iota(indices.begin(), indices.end(), 0);
  std::sort(indices.begin(), indices.end(), [&](size_t i1, size_t i2) {
    return sortIndices[i1] < sortIndices[i2];
  });

  std::vector<std::remove_cvref_t<decltype(arr[0])>> sortedMatrices;
  for (auto i : indices) {
    sortedMatrices.push_back(arr[i]);
  }
  return sortedMatrices;
}

// Suppose our sort index array looks like
//
// 0  1 -1  2  3  4 -1 -1  5
// Our sortBiasCtr and lastSort variable logic will instead produce
// 0  1  2  3  4  5  6  7  8
//
static inline void RemoveSortPlaceholdersInplace(auto& sortIndices) {
  // TODO: static_assert sortIndices is arrar of a signed type
  int sortBiasCtr = 0;
  int lastSort = 0;
  for (auto& sortIndex : sortIndices) {
    // negative: placeholder
    if (sortIndex < 0) {
      sortIndex = lastSort + 1;
      ++sortBiasCtr;
    } else {
      sortIndex += sortBiasCtr;
    }
    lastSort = sortIndex;
  }
}

} // namespace rsl
