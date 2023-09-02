#pragma once

#include <type_traits>
#include <unordered_map>
#include <vector>

namespace rsl {

template <typename T> struct CompactedVector {
  std::vector<T> uniqueElements;
  // TODO: This can just be a vector
  std::unordered_map<size_t, size_t> remapTableOldToNew;
};
[[nodiscard]] static inline auto StableCompactVector(auto& tracks) {
  std::vector<std::remove_cvref_t<decltype(tracks[0])>> uniqueTracks;
  std::unordered_map<size_t, size_t> trackIndexMap;

  for (size_t i = 0; i < tracks.size(); ++i) {
    // TODO: Consider using a set or stable_sort to get better asymptotic
    // performance here
    auto it = std::find(uniqueTracks.begin(), uniqueTracks.end(), tracks[i]);
    if (it == uniqueTracks.end()) {
      // The track is not found in the uniqueTracks, so we add it
      uniqueTracks.push_back(tracks[i]);
      trackIndexMap[i] = uniqueTracks.size() - 1;
    } else {
      // The track is found in uniqueTracks, so we map the old index to the
      // found index
      trackIndexMap[i] = std::distance(uniqueTracks.begin(), it);
    }
  }

  return CompactedVector<std::remove_cvref_t<decltype(tracks[0])>>{
      .uniqueElements = uniqueTracks,
      .remapTableOldToNew = trackIndexMap,
  };
}

} // namespace rsl
