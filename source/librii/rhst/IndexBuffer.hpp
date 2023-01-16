#pragma once

#include <librii/rhst/RHST.hpp>

namespace librii::rhst {

// As RHST isn't an indexed format (yet), this class does the conversion.
template <typename T = u32> struct IndexBuffer {
  static Result<IndexBuffer<T>> create(const MatrixPrimitive& prim) {
    EXPECT(prim.primitives.size() == 1);
    EXPECT(prim.primitives[0].topology == Topology::Triangles);
    IndexBuffer<T> tmp;
    for (auto& v : prim.primitives[0].vertices) {
      auto it = std::find(tmp.vertices.begin(), tmp.vertices.end(), v);
      if (it != tmp.vertices.end()) {
        tmp.index_data.push_back(it - tmp.vertices.begin());
        continue;
      }
      tmp.index_data.push_back(static_cast<T>(tmp.vertices.size()));
      tmp.vertices.push_back(v);
      EXPECT(tmp.vertices[tmp.vertices.size() - 1] == v);
    }
    return tmp;
  }
  std::vector<Vertex> vertices;
  std::vector<T> index_data;
};

} // namespace librii::rhst
