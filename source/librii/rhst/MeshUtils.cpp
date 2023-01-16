#include "MeshUtils.hpp"

namespace librii::rhst {

coro::generator<Result<size_t>>
MeshUtils::AsTrianglesIdx(const Primitive& prim) {
  switch (prim.topology) {
  case Topology::TriangleStrip: {
    //
    // TRIANGLE STRIPS
    //
    // A) Vertices [0, 2]. Define the initial triangle:
    //      v0--v2
    //      |  /
    //      | /
    //      v1
    //
    if (prim.vertices.size() < 3) {
      co_yield std::unexpected("Invalid triangle strip size");
    }
    for (size_t v = 0; v < 3; ++v) {
      co_yield v;
    }
    // B) Every next vertex n. Define a subsequent triangle:
    //      v0--v2
    //      |  / \
    //      | /   \
    //      v1-----v3
    //
    //      v0--v2----v4
    //      |  / \   /
    //      | /   \ /
    //      v1-----v3
    //
    //      v0--v2---v4
    //      |  / \   /\
    //      | /   \ /  \
    //      v1-----v3--v5
    //
    for (size_t v = 3; v < prim.vertices.size(); ++v) {
      co_yield v - ((v & 1) ? 1 : 2);
      co_yield v - ((v & 1) ? 2 : 1);
      co_yield v;
    }
    co_return;
  }
  case Topology::TriangleFan: {
    //
    // TRIANGLE FANS
    //
    // A) Vertices [0, 2]. Define the initial triangle:
    //      v0--v2
    //      | /
    //      |/
    //      v1
    //
    if (prim.vertices.size() < 3) {
      co_yield std::unexpected("Invalid triangle fan size");
    }
    for (size_t v = 0; v < 3; ++v) {
      co_yield v;
    }
    // B) Every next vertex n. Define a subsequent triangle:
    //         v3
    //        / |
    //       /  |
    //      v0--v2
    //      |  /
    //      | /
    //      v1
    //
    //   v4----v3
    //     \  / |
    //      \/  |
    //      v0--v2
    //      |  /
    //      | /
    //      v1
    //
    //  v4-----v3
    //   | \  / |
    //   |  \/  |
    //  v5--v0--v2
    //      |  /
    //      | /
    //      v1
    //
    for (size_t v = 3; v < prim.vertices.size(); ++v) {
      co_yield 0;
      co_yield v - 1;
      co_yield v;
    }
    co_return;
  }
  case Topology::Triangles:
    //
    // TRIANGLES
    //
    // A) Vertices [n, n+2]. Define a triangle:
    //      v0--v2
    //      |  /
    //      | /
    //      v1
    //
    //      v0--v2  v4--v5
    //      |  /     \  |
    //      | /       \ |
    //      v1         v4
    //
    //      v0--v2  v4--v3        v6
    //      |  /     \  |        / |
    //      | /       \ |       /  |
    //      v1         v5     v7--v8
    //
    if (prim.vertices.size() % 3 != 0) {
      co_yield std::unexpected("Invalid triangle size");
    }
    for (size_t i = 0; i < prim.vertices.size(); ++i) {
      co_yield i;
    }
    co_return;
  default:
    break;
  }
  co_yield std::unexpected("Unexpected primitive type");
}

coro::generator<Result<Vertex>>
MeshUtils::AsTriangles(std::span<const Primitive> primitives) {
  for (auto& prim : primitives) {
    for (auto idx : AsTrianglesIdx(prim)) {
      if (!idx.has_value()) {
        co_yield std::unexpected(idx.error());
        co_return;
      }
      co_yield prim.vertices[*idx];
    }
  }
}

Result<MatrixPrimitive>
MeshUtils::TriangulateMPrim(const MatrixPrimitive& prim) {
  MatrixPrimitive result = prim;
  result.primitives.clear();
  auto& tris = result.primitives.emplace_back();
  tris.topology = Topology::Triangles;
  for (auto vert : AsTriangles(prim.primitives)) {
    tris.vertices.push_back(TRY(vert));
  }
  return result;
}

Result<Mesh> MeshUtils::TriangulateMesh(const Mesh& mesh) {
  Mesh result = mesh;
  result.matrix_primitives.clear();
  for (auto& mprim : mesh.matrix_primitives) {
    result.matrix_primitives.push_back(TRY(TriangulateMPrim(mprim)));
  }
  return result;
}

} // namespace librii::rhst
