#pragma once

#include <core/common.h>
#include <coro/generator.hpp>
#include <librii/rhst/RHST.hpp>

namespace librii::rhst {

class MeshUtils {
public:
  // Splits a list of indices |mesh| by a delimeter |primitive_restart_index|.
  // https://www.khronos.org/opengl/wiki/Vertex_Rendering#Primitive_Restart
  template <typename IndexTypeT>
  static coro::generator<std::span<const IndexTypeT>>
  SplitByPrimitiveRestart(std::span<const IndexTypeT> mesh,
                          IndexTypeT primitive_restart_index) {
    size_t last = 0;
    for (size_t i = 0; i <= mesh.size(); ++i) {
      if (mesh[i] == primitive_restart_index || i == mesh.size()) {
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

  // Returns a coroutine that yields the indices of a given input primitive
  // |prim| in triangle form.
  static coro::generator<Result<size_t>> AsTrianglesIdx(const Primitive& prim);

  // Transforms a list of primitives of any type |primitives| to a list of
  // just triangles.
  static coro::generator<Result<Vertex>>
  AsTriangles(std::span<const Primitive> primitives);

  // Attempts to triangulate a MatrixPrimitive |prim|.
  [[nodiscard]] static Result<MatrixPrimitive>
  TriangulateMPrim(const MatrixPrimitive& prim);

  // Attempts to triangulate a Mesh |mesh|.
  [[nodiscard]] static Result<Mesh> TriangulateMesh(const Mesh& mesh);
};

} // namespace librii::rhst
