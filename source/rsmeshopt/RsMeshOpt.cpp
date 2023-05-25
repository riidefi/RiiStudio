#include <core/common.h>

#include <rsmeshopt/TriStripper/tri_stripper.h>
#include <rsmeshopt/tristrip/tristrip.hpp>

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

} // namespace rsmeshopt
