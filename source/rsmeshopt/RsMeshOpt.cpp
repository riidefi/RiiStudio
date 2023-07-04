#include <core/common.h>

#include <rsmeshopt/TriStripper/tri_stripper.h>
#include <rsmeshopt/tristrip/tristrip.hpp>

#include <meshoptimizer.h>

namespace rsmeshopt {

Result<std::vector<std::vector<u32>>>
StripifyTrianglesNvTriStripPort(std::span<const u32> index_data) {
  EXPECT(index_data.size() % 3 == 0);
  std::list<std::list<int>> mesh;
  for (size_t i = 0; i < index_data.size(); i += 3) {
    std::list<int> tri;
    tri.push_back(index_data[i]);
    tri.push_back(index_data[i + 1]);
    tri.push_back(index_data[i + 2]);
    mesh.push_back(std::move(tri));
  }
  std::list<std::deque<int>> strips = stripify(mesh);

  std::vector<std::vector<u32>> result;
  for (auto& s : strips) {
    auto& tmp = result.emplace_back();
    for (auto v : s) {
      EXPECT(v >= 0);
      tmp.push_back(static_cast<u32>(v));
    }
  }
  return result;
}

Result<triangle_stripper::primitive_vector>
StripifyTrianglesTriStripper(std::span<const u32> index_data) {
  EXPECT(index_data.size() % 3 == 0);
  std::vector<size_t> buffer(index_data.begin(), index_data.end());

  triangle_stripper::tri_stripper stripper(buffer);
  triangle_stripper::primitive_vector out;
  stripper.Strip(&out);
  return out;
}

size_t meshopt_stripify(unsigned int* destination, const unsigned int* indices,
                        size_t index_count, size_t vertex_count,
                        unsigned int restart_index) {
  return ::meshopt_stripify(destination, indices, index_count, vertex_count,
                            restart_index);
}
size_t meshopt_stripifyBound(size_t index_count) {
  return ::meshopt_stripifyBound(index_count);
}
size_t meshopt_unstripify(unsigned int* destination,
                          const unsigned int* indices, size_t index_count,
                          unsigned int restart_index) {
  return ::meshopt_unstripify(destination, indices, index_count, restart_index);
}
size_t meshopt_unstripifyBound(size_t index_count) {
  return ::meshopt_unstripifyBound(index_count);
}

} // namespace rsmeshopt
