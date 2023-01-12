#include "RHST.hpp"
#include <draco/mesh/mesh_stripifier.h>
#include <meshoptimizer.h>
#include <vendor/TriStripper/tri_stripper.h>
#include <vendor/tristrip/tristrip.hpp>

namespace librii::rhst {

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

Result<void> StripifyTrianglesMeshOptimizer(MatrixPrimitive& prim) {
  auto buf = TRY(IndexBuffer<u32>::create(prim));
  std::vector<Vertex> vertices = std::move(buf.vertices);
  std::vector<u32> index_data = std::move(buf.index_data);

  size_t index_count = index_data.size();
  size_t vertex_count = vertices.size();
  std::vector<unsigned int> strip(meshopt_stripifyBound(index_count));
  unsigned int restart_index = ~0u;
  size_t strip_size = meshopt_stripify(
      &strip[0], index_data.data(), index_count, vertex_count, restart_index);
  u32 real_size = 0;
  for (u32 i = 0; i < strip_size; ++i) {
    if (strip[i] != ~0u) {
      ++real_size;
    }
  }
  auto msg =
      std::format("Stripped: Before {} After {}. Reduction of {} percent",
                  index_count, real_size,
                  100.0f * (1.0f - static_cast<float>(real_size) /
                                       static_cast<float>(index_count)));
  fprintf(stderr, "%s\n", msg.c_str());

  std::vector<Primitive> prims;
  Primitive* p = nullptr;
  for (u32 i = 0; i < strip_size; ++i) {
    u32 idx = strip[i];
    if (i == 0 || idx == ~0u) {
      p = nullptr;
      prims.emplace_back();
      p = &prims[prims.size() - 1];
      p->topology = Topology::TriangleStrip;
      if (idx == ~0u) {
        continue;
      }
    }
    EXPECT(idx < vertices.size());
    p->vertices.push_back(vertices[idx]);
  }
  prim.primitives.clear();
  for (auto& x : prims) {
    EXPECT(x.vertices.size() >= 3);
    if (x.vertices.size() > 3) {
      prim.primitives.push_back(x);
    }
  }
  auto& v_new = prim.primitives.emplace_back();
  v_new.topology = Topology::Triangles;
  for (auto& x : prims) {
    if (x.vertices.size() == 3) {
      for (auto& y : x.vertices) {
        v_new.vertices.push_back(y);
      }
    }
  }
  if (v_new.vertices.size() == 0) {
    prim.primitives.resize(prim.primitives.size() - 1);
  }
  return {};
}
Result<void> StripifyTrianglesTriStripper(MatrixPrimitive& prim) {
  auto buf = TRY(IndexBuffer<size_t>::create(prim));
  std::vector<Vertex> vertices = std::move(buf.vertices);
  std::vector<size_t> index_data = std::move(buf.index_data);

  size_t index_count = index_data.size();

  triangle_stripper::tri_stripper stripper(index_data);
  triangle_stripper::primitive_vector out;
  stripper.Strip(&out);

  size_t real_size = 0;
  for (auto& p : out) {
    real_size += p.Indices.size();
  }
  auto msg =
      std::format("Stripped: Before {} After {}. Reduction of {} percent",
                  index_count, real_size,
                  100.0f * (1.0f - static_cast<float>(real_size) /
                                       static_cast<float>(index_count)));
  fprintf(stderr, "%s\n", msg.c_str());

  prim.primitives.clear();
  for (auto& x : out) {
    auto& to = prim.primitives.emplace_back();
    switch (x.Type) {
    case triangle_stripper::TRIANGLES:
      to.topology = Topology::Triangles;
      break;
    case triangle_stripper::TRIANGLE_STRIP:
      to.topology = Topology::TriangleStrip;
      break;
    }
    for (size_t idx : x.Indices) {
      to.vertices.push_back(vertices[idx]);
    }
  }

  return {};
}
Result<void> StripifyTrianglesNvTriStripPort(MatrixPrimitive& prim) {
  auto buf = TRY(IndexBuffer<u32>::create(prim));
  std::vector<Vertex> vertices = std::move(buf.vertices);
  std::vector<u32> index_data = std::move(buf.index_data);

  EXPECT(index_data.size() % 3 == 0);
  std::list<std::list<int>> mesh;
  for (size_t i = 0; i < index_data.size(); i += 3) {
    std::list<int> tri;
    tri.push_back(index_data[i]);
    tri.push_back(index_data[i + 1]);
    tri.push_back(index_data[i + 2]);
    mesh.push_back(std::move(tri));
  }
  size_t index_count = index_data.size();

  auto strips = stripify(mesh);

  size_t real_size = 0;
  for (auto& p : strips) {
    real_size += p.size();
  }
  auto msg =
      std::format("Stripped: Before {} After {}. Reduction of {} percent",
                  index_count, real_size,
                  100.0f * (1.0f - static_cast<float>(real_size) /
                                       static_cast<float>(index_count)));
  fprintf(stderr, "%s\n", msg.c_str());

  prim.primitives.clear();
  for (auto& x : strips) {
    EXPECT(x.size() >= 3);
    if (x.size() <= 3)
      continue;
    auto& to = prim.primitives.emplace_back();
    to.topology = Topology::TriangleStrip;
    for (int idx : x) {
      to.vertices.push_back(vertices[idx]);
    }
  }
  auto& v_new = prim.primitives.emplace_back();
  v_new.topology = Topology::Triangles;
  for (auto& x : strips) {
    if (x.size() == 3) {
      for (int y : x) {
        v_new.vertices.push_back(vertices[y]);
      }
    }
  }
  if (v_new.vertices.size() == 0) {
    prim.primitives.resize(prim.primitives.size() - 1);
  }

  return {};
}

namespace HaroohiePals {

class Primitive {
public:
  std::string type = "triangles";
  bool processed = false;
  int next_candidate_count;
  std::vector<int> next_candidates{-1, -1, -1, -1};

  std::vector<int> vertex_indices;
  void add_vtx(Primitive& primitive, int i) {
    vertex_indices.push_back(primitive.vertex_indices[i]);
  }

public:
  bool is_extra_data_equal(int a, const Primitive& other, int b) const {
    return vertex_indices[a] == other.vertex_indices[b];
  }
  bool is_suitable_tstrip_candidate(const Primitive& candidate) {
    int equal_count = 0;
    int first_i = 0;
    int first_j = 0;
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        if (!is_extra_data_equal(i, candidate, j)) {
          continue;
        }
        if (equal_count == 0) {
          first_i = i;
          first_j = j;
        } else if (equal_count == 1) {
          if (first_i == 0 && i == 2) {
            return first_j < j || (first_j == 2 && j == 0);
          }
          return first_j > j || (first_j == 0 && j == 2);
        }
        equal_count++;
      }
    }
    return false;
  }
  bool is_suitable_tstrip_candidate_edge(const Primitive& candidate, int a,
                                         int b) {
    int equal_count = 0;
    for (int i = 0; i < 3; i++) {
      if (is_extra_data_equal(a, candidate, i)) {
        equal_count++;
      }
      if (is_extra_data_equal(b, candidate, i)) {
        equal_count++;
      }
    }
    return equal_count == 2;
  }
};
class TriStripper {
public:
  std::pair<int, int> get_previous_tri_edge(int a, int b) {
    if (b == 0) {
      return {2 - (a == 1 ? 0 : 1), a};
    }
    if (b == 1) {
      return {a == 2 ? 0 : 2, a};
    }
    return {a == 0 ? 1 : 0, a};
  }
  int try_strip_in_direction(std::vector<Primitive>& tris, int tri_idx,
                             int vtxa, int vtxb) {
    std::vector<bool> processed_tmp;
    for (const auto& tri : tris) {
      processed_tmp.push_back(tri.processed);
    }
    processed_tmp[tri_idx] = true;
    Primitive tri = tris[tri_idx];
    std::tie(vtxb, vtxa) = get_previous_tri_edge(vtxb, vtxa);
    int tri_count = 1;
    while (true) {
      int i = -1;
      while (i < 3) {
        i++;
        if (i >= 3) {
          break;
        }
        if (tri.next_candidates[i] == -1) {
          continue;
        }
        Primitive& candidate = tris[tri.next_candidates[i]];
        if (processed_tmp[tri.next_candidates[i]]) {
          continue;
        }
        if (!tri.is_suitable_tstrip_candidate_edge(candidate, vtxa, vtxb)) {
          continue;
        }
        // TODO: Just use position vector?
        auto pos_a = tri.vertex_indices[vtxa];
        vtxa = 0;
        while (vtxa < 3) {
          if (candidate.vertex_indices[vtxa] == pos_a) {
            break;
          }
          vtxa++;
        }
        auto pos_b = tri.vertex_indices[vtxb];
        vtxb = 0;
        while (vtxb < 3) {
          if (candidate.vertex_indices[vtxb] == pos_b) {
            break;
          }
          vtxb++;
        }
        if (vtxa != 3 && vtxb != 3) {
          std::tie(vtxb, vtxa) = get_previous_tri_edge(vtxb, vtxa);
          processed_tmp[tri.next_candidates[i]] = true;
          tri_count++;
          tri = candidate;
          break;
        }
      }
      if (i == 3) {
        break;
      }
    }
    return tri_count;
  }
  Primitive make_tstrip_primitive(std::vector<Primitive>& tris, int tri_idx,
                                  int vtxa, int vtxb) {
    Primitive result;
    result.type = "triangle_strip";
    Primitive tri = tris[tri_idx];
    tris[tri_idx].processed = true;
    result.add_vtx(tri, vtxa);
    result.add_vtx(tri, vtxb);
    std::tie(vtxb, vtxa) = get_previous_tri_edge(vtxb, vtxa);
    result.add_vtx(tri, vtxb);
    while (true) {
      int i = -1;
      while (i < 3) {
        i++;
        if (i >= 3) {
          break;
        }
        if (tri.next_candidates[i] == -1) {
          continue;
        }
        Primitive& candidate = tris[tri.next_candidates[i]];
        if (candidate.processed) {
          continue;
        }
        if (!tri.is_suitable_tstrip_candidate_edge(candidate, vtxa, vtxb)) {
          continue;
        }
        // TODO: Just check positions?
        auto pos_a = tri.vertex_indices[vtxa];
        vtxa = 0;
        while (vtxa < 3) {
          if (candidate.vertex_indices[vtxa] == pos_a) {
            break;
          }
          vtxa++;
        }
        auto pos_b = tri.vertex_indices[vtxb];
        vtxb = 0;
        while (vtxb < 3) {
          if (candidate.vertex_indices[vtxb] == pos_b) {
            break;
          }
          vtxb++;
        }
        if (vtxa != 3 && vtxb != 3) {
          std::tie(vtxb, vtxa) = get_previous_tri_edge(vtxb, vtxa);
          result.add_vtx(candidate, vtxb);
          candidate.processed = true;
          tri = candidate;
          break;
        }
      }
      if (i == 3) {
        break;
      }
    }
    return result;
  }
  std::vector<Primitive> process(std::vector<Primitive> primitives) {
    std::vector<Primitive> result;
    std::vector<Primitive> tris;
    for (Primitive& tri : primitives) {
      if (tri.type == "triangles") {
        tri.processed = false;
        tris.push_back(tri);
      }
    }
    for (Primitive& tri : tris) {
      tri.next_candidate_count = 0;
      tri.next_candidates = {-1, -1, -1, -1};
      for (int i = 0; i < tris.size(); i++) {
        Primitive& candidate = tris[i];
        if (!tri.is_suitable_tstrip_candidate(candidate)) {
          continue;
        }
        tri.next_candidates[tri.next_candidate_count] = i;
        tri.next_candidate_count++;
        if (tri.next_candidate_count >= 3) {
          break;
        }
      }
    }
    while (true) {
      int count = 0;
      for (Primitive& tri : tris) {
        if (tri.processed) {
          continue;
        }
        count++;
        if (tri.next_candidate_count > 0) {
          int cand_count = 0;
          for (auto& c : tri.next_candidates) {
            if (c != -1 && !tris[c].processed) {
              ++cand_count;
            }
          }
          tri.next_candidate_count = cand_count;
        }
      }
      if (count == 0) {
        break;
      }
      int min_cand_count_idx = -1;
      int min_cand_count = std::numeric_limits<int>::max();
      for (int i = 0; i < tris.size(); i++) {
        Primitive& tri = tris[i];
        if (tri.processed) {
          continue;
        }
        if (tri.next_candidate_count < min_cand_count) {
          min_cand_count = tri.next_candidate_count;
          min_cand_count_idx = i;
          if (min_cand_count <= 1) {
            break;
          }
        }
      }
      int max_tris = 0;
      int max_tris_vtx0 = -1;
      int max_tris_vtx1 = -1;
      for (int i = 0; i < 3; i++) {
        int vtx0 = i;
        int vtx1 = (i == 2) ? 0 : i + 1;
        int tri_count =
            try_strip_in_direction(tris, min_cand_count_idx, vtx0, vtx1);
        if (tri_count > max_tris) {
          max_tris = tri_count;
          max_tris_vtx0 = vtx0;
          max_tris_vtx1 = vtx1;
        }
      }
      if (max_tris <= 1) {
        Primitive& tri = tris[min_cand_count_idx];
        tri.processed = true;
        result.push_back(tri);
      } else {
        result.push_back(make_tstrip_primitive(tris, min_cand_count_idx,
                                               max_tris_vtx0, max_tris_vtx1));
      }
    }
    // Grab existing strips (none)
    for (auto primitive : primitives) {
      if (primitive.type != "triangles") {
        result.push_back(primitive);
      }
    }
    return result;
  }
};

// If indices.size() == 3 -> TRIANGLES; else TRIANGLE_STRIP. No quad support.
std::vector<std::vector<int>> TriangleStrip(std::span<const int> indices) {
  assert(indices.size() % 3 == 0);
  std::vector<Primitive> primitives;
  for (size_t i = 0; i < indices.size(); i += 3) {
    Primitive prim{};
    prim.vertex_indices.push_back(indices[i]);
    prim.vertex_indices.push_back(indices[i + 1]);
    prim.vertex_indices.push_back(indices[i + 2]);
    primitives.push_back(std::move(prim));
  }
  TriStripper stripper;
  auto processed = stripper.process(primitives);
  std::vector<std::vector<int>> result;
  for (auto& prim : processed) {
    result.push_back(prim.vertex_indices);
  }
  return result;
}
} // namespace HaroohiePals

Result<void> StripifyTrianglesHaroohie(MatrixPrimitive& prim) {
  auto buf = TRY(IndexBuffer<int>::create(prim));
  std::vector<Vertex> vertices = std::move(buf.vertices);
  std::vector<int> index_data = std::move(buf.index_data);

  EXPECT(index_data.size() % 3 == 0);
  size_t index_count = index_data.size();

  auto strips = HaroohiePals::TriangleStrip(index_data);

  size_t real_size = 0;
  for (auto& p : strips) {
    real_size += p.size();
  }
  auto msg =
      std::format("Stripped: Before {} After {}. Reduction of {} percent",
                  index_count, real_size,
                  100.0f * (1.0f - static_cast<float>(real_size) /
                                       static_cast<float>(index_count)));
  fprintf(stderr, "%s\n", msg.c_str());

  prim.primitives.clear();
  for (auto& x : strips) {
    EXPECT(x.size() >= 3);
    if (x.size() <= 3)
      continue;
    auto& to = prim.primitives.emplace_back();
    to.topology = Topology::TriangleStrip;
    for (int idx : x) {
      to.vertices.push_back(vertices[idx]);
    }
  }
  auto& v_new = prim.primitives.emplace_back();
  v_new.topology = Topology::Triangles;
  for (auto& x : strips) {
    if (x.size() == 3) {
      for (int y : x) {
        v_new.vertices.push_back(vertices[y]);
      }
    }
  }
  if (v_new.vertices.size() == 0) {
    prim.primitives.resize(prim.primitives.size() - 1);
  }

  return {};
}

Result<void> StripifyTrianglesDraco(MatrixPrimitive& prim, bool degen) {
  auto buf = TRY(IndexBuffer<u32>::create(prim));
  std::vector<Vertex> vertices = std::move(buf.vertices);
  std::vector<u32> index_data = std::move(buf.index_data);
  assert(index_data.size() % 3 == 0);
  size_t index_count = index_data.size();
  size_t vertex_count = vertices.size();

  draco::Mesh mesh;
  for (size_t i = 0; i < index_data.size(); i += 3) {
    std::array<draco::PointIndex, 3> face;
    face[0] = index_data[i];
    face[1] = index_data[i + 1];
    face[2] = index_data[i + 2];
    mesh.AddFace(face);
  }
  auto pos = std::make_unique<draco::PointAttribute>();
  pos->Init(draco::GeometryAttribute::POSITION, 3, draco::DT_FLOAT32, false,
            vertex_count);
  auto other = std::make_unique<draco::PointAttribute>();
  other->Init(draco::GeometryAttribute::GENERIC, 1, draco::DT_UINT32, false,
              vertex_count);
  for (size_t i = 0; i < vertices.size(); ++i) {
    pos->SetAttributeValue(draco::AttributeValueIndex(i), &vertices[i]);
    u32 tmp = i;
    other->SetAttributeValue(draco::AttributeValueIndex(i), &tmp);
  }
  mesh.SetAttribute(0, std::move(pos));
  mesh.SetAttribute(1, std::move(other));
  draco::MeshStripifier stripifier;
  std::vector<u32> strip;
  bool ok = false;
  if (degen) {
    ok = stripifier.GenerateTriangleStripsWithDegenerateTriangles(
        mesh, std::back_inserter(strip));
  } else {
    ok = stripifier.GenerateTriangleStripsWithPrimitiveRestart(
        mesh, ~0u, std::back_inserter(strip));
  }
  assert(ok);
  u32 strip_size = strip.size();
  u32 real_size = 0;
  for (u32 i = 0; i < strip_size; ++i) {
    if (strip[i] != ~0u) {
      ++real_size;
    }
  }
  auto msg =
      std::format("Stripped: Before {} After {}. Reduction of {} percent",
                  index_count, real_size,
                  100.0f * (1.0f - static_cast<float>(real_size) /
                                       static_cast<float>(index_count)));
  fprintf(stderr, "%s\n", msg.c_str());

  std::vector<Primitive> prims;
  Primitive* p = nullptr;
  for (u32 i = 0; i < strip_size; ++i) {
    u32 idx = strip[i];
    if (i == 0 || idx == ~0u) {
      p = nullptr;
      prims.emplace_back();
      p = &prims[prims.size() - 1];
      p->topology = Topology::TriangleStrip;
      if (idx == ~0u) {
        continue;
      }
    }
    EXPECT(idx < vertices.size());
    p->vertices.push_back(vertices[idx]);
  }
  prim.primitives.clear();
  for (auto& x : prims) {
    EXPECT(x.vertices.size() >= 3);
    if (x.vertices.size() > 3) {
      prim.primitives.push_back(x);
    }
  }
  auto& v_new = prim.primitives.emplace_back();
  v_new.topology = Topology::Triangles;
  for (auto& x : prims) {
    if (x.vertices.size() == 3) {
      for (auto& y : x.vertices) {
        v_new.vertices.push_back(y);
      }
    }
  }
  if (v_new.vertices.size() == 0) {
    prim.primitives.resize(prim.primitives.size() - 1);
  }
  return {};
}

} // namespace librii::rhst
