#pragma once

#include <core/common.h>

namespace librii::rhst {

// Splits a mesh containing a central vertex into many sub-graphs of "islands"
// such that each could be a valid trifan.
class TriangleFanSplitter {
public:
  TriangleFanSplitter() = default;
  ~TriangleFanSplitter() = default;

  // Generate a list of islands (specified by their face index) from the subset
  // of |mesh| (indexed vertex data in triangle form) such that each triangle is
  // adjacent to vertex |center|.
  //
  // The triangle can be retrieved from the face index as
  //    [ mesh[face], mesh[face+1], mesh[face_2] ]
  //
  std::vector<std::set<size_t>> ConvertToFans(std::span<const u32> mesh,
                                              u32 center);

private:
  // Determines if a given fan can attach the following face
  bool CanAddToFan(const std::set<size_t>& island,
                   std::span<const size_t, 3> face);

  std::span<const u32> mesh_{};
  u32 center_{};

  std::vector<std::set<size_t>> islands_{};
};

} // namespace librii::rhst
