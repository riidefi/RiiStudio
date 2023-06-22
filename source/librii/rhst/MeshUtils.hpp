#pragma once

#include <core/common.h>
#include <coro/generator.hpp>
#include <librii/rhst/RHST.hpp>

namespace librii::rhst {

class MeshUtils {
public:
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
