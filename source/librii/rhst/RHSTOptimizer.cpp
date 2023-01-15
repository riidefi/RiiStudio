#include "RHST.hpp"
#include "RingIterator.hpp"

#include <draco/mesh/mesh_stripifier.h>
#include <meshoptimizer.h>
#include <vendor/TriStripper/tri_stripper.h>
#include <vendor/tristrip/tristrip.hpp>

#include <fmt/color.h>
#include <rsl/Ranges.hpp>

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

static void LogDone(u32 index_count, u32 real_size) {
  fmt::print(stderr, "\t\tBefore \t{} After \t{}. \tReduction of ", index_count,
             real_size);
  fmt::print(stderr, fmt::fg(fmt::color::red), "{}",
             100.0f * (1.0f - static_cast<float>(real_size) /
                                  static_cast<float>(index_count)));
  fmt::print(stderr, "\tpercent\n");
}

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
  LogDone(index_count, real_size);

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
  LogDone(index_count, real_size);

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
  LogDone(index_count, real_size);

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
  LogDone(index_count, real_size);

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

static bool TriangleArrayHoldsDuplicates(std::span<const u32> index_data) {
  std::vector<std::array<size_t, 3>> cache;
  for (size_t i = 0; i < index_data.size(); i += 3) {
    std::array<size_t, 3> cand{index_data[i], index_data[i + 1],
                               index_data[i + 2]};
    auto it = std::ranges::find(cache, cand);
    if (it == cache.end()) {
      cache.push_back(cand);
      continue;
    }
    return true;
  }
  return false;
}

// Splits a mesh containing a central vertex into many sub-graphs of "islands"
// such that each could be a valid trifan.
class TriangleFanSplitter {
public:
  TriangleFanSplitter(std::span<const u32> mesh, u32 center)
      : mesh_(mesh), center_(center) {}

  std::vector<std::set<size_t>> ConvertToFans() {
    islands_.clear();
    // Stores the first vertex of the face
    std::set<size_t> candidate_faces;
    for (size_t i = 0; i < mesh_.size(); ++i) {
      if (mesh_[i] == center_) {
        candidate_faces.insert((i / 3) * 3);
      }
    }
    for (size_t cand_face : candidate_faces) {
      std::array<size_t, 3> cand_face_indices = {
          mesh_[cand_face],
          mesh_[cand_face + 1],
          mesh_[cand_face + 2],
      };
      std::set<size_t>* it = nullptr;
      for (auto& island : islands_) {
        if (CanAddToFan(island, cand_face_indices)) {
          it = &island;
          break;
        }
      }
      if (it == nullptr) {
        // No existing island: create a new one
        std::set<size_t> island;
        island.insert(cand_face);
        islands_.push_back(island);
      } else {
        it->insert(cand_face);
      }
    }
    return islands_;
  }

private:
  bool CanAddToFan(const std::set<size_t>& island,
                   std::span<const size_t, 3> face) {
    // Subgraph degree/2 by index_data[...]
    // TODO: Precompute
    std::unordered_map<std::size_t, std::size_t> valence_cache;
    for (auto& island_face : island) {
      ++valence_cache[mesh_[island_face]];
      ++valence_cache[mesh_[island_face + 1]];
      ++valence_cache[mesh_[island_face + 2]];
    }
    for (auto& island_face : island) {
      std::array<size_t, 3> island_face_indices = {
          mesh_[island_face],
          mesh_[island_face + 1],
          mesh_[island_face + 2],
      };
      for (int i = 0; i < 3; ++i) {
        int cand_edge_from = face[(i + 2) % 3];
        int cand_vert = face[i];
        int cand_edge_to = face[(i + 1) % 3];
        for (int j = 0; j < 3; ++j) {
          int island_edge_from = island_face_indices[(j + 2) % 3];
          int island_vert = island_face_indices[j];
          int island_edge_to = island_face_indices[(j + 1) % 3];
          if (cand_vert == island_vert) {
            // The center will always match
            if (cand_vert == center_) {
              continue;
            }
            // Check the winding order actually matches
            if (cand_edge_from != island_edge_to &&
                cand_edge_to != island_edge_from) {
#if LIBRII_RINGITERATOR_DEBUG
              fmt::print(stderr,
                         "Candidate {} is incorrect winding order (island "
                         "tri: ({} -> {} -> {}) cand tri ({} -> {} -> {})\n",
                         cand_vert, island_edge_from, island_vert,
                         island_edge_to, cand_edge_from, cand_vert,
                         cand_edge_to);
              // assert(false);
#endif
              continue;
            }

            if (valence_cache[cand_vert] >= 2) {
              // Basically this diagram. We can't add to island in this
              // case.
              //
              // https://cdn.discordapp.com/attachments/337714434262827008/1063549172357283920/image.png
              //
#if LIBRII_RINGITERATOR_DEBUG
              fmt::print(stderr,
                         "Weird modeling thing. Valence of {} attempting to "
                         "add another\n",
                         island_valency[cand_vert]);
#endif
              continue;
            }
#if LIBRII_RINGITERATOR_DEBUG
            fmt::print(stderr,
                       "Cand {}: Match found (island "
                       "tri: ({} -> {} -> {}) cand tri ({} -> {} -> {})\n",
                       cand_vert, island_edge_from, island_vert, island_edge_to,
                       cand_edge_from, cand_vert, cand_edge_to);
#endif
            return true;
          }
        }
      }
    }

    return false;
  }

  std::span<const u32> mesh_{};
  u32 center_{};

  std::vector<std::set<size_t>> islands_{};
};

struct TriFanOptions {
  // Should never go lower, as a 3-triangle fan is also a strip.
  u32 min_fan_size = 4;
};

// Class that generates triangle fans from a provided indexed triangle mesh
// array. The fans represent a more memory-efficient storage of triangle
// connectivity that can be used directly on the GPU. In general, a mesh needs
// to be represented by several triangle fans/strips. The algorithm here is
// basic.
class TriFanPass {
public:
  TriFanPass() = default;
  ~TriFanPass() = default;

  template <typename OutputIteratorT, typename IndexTypeT>
  bool GenerateTriangleFansWithPrimitiveRestart(
      std::span<const u32> mesh, IndexTypeT primitive_restart_index,
      OutputIteratorT out_it, TriFanOptions options = {});

  bool Prepare(std::span<const u32> mesh, const TriFanOptions& options) {
    assert(mesh.size() % 3 == 0);
    assert(!TriangleArrayHoldsDuplicates(mesh) && "Duplicate triangles");
    mesh_ = mesh;
    face_visited_.clear();
    face_visited_.resize(mesh.size() / 3);
    std::set<u32> verts(mesh.begin(), mesh.end());
    num_vertices_ = verts.size();
    valence_cache_.clear();
    valence_cache_.resize(num_vertices_);
    for (u32 vertex : mesh_) {
      ++valence_cache_[vertex];
    }
    min_fan_size_ = options.min_fan_size;
    return true;
  }

  template <typename OutputIteratorT>
  void StoreFan(size_t center, std::span<const u32> fan,
                OutputIteratorT out_it) {
    ++num_fans_;
    std::vector<size_t> island_;
    for (size_t face : fan) {
      face_visited_[face / 3] = true;
      island_.push_back(mesh_[face]);
      island_.push_back(mesh_[face + 1]);
      island_.push_back(mesh_[face + 2]);
    }

    // Update valence cache
    for (size_t vert : island_) {
      --valence_cache_[vert];
    }

    // TODO: Debugging: Enable to skip RingIterator and just assess islands
    // TODO: assert(fan.vertices.size() == island_.size() / 3 + 2);
    *out_it++ = center;

    RingIterator<size_t> ring_iterator(center, island_);
    for (auto vtx : ring_iterator) {
      *out_it++ = vtx;
    }
  }

  std::vector<std::vector<u32>>
  FindFansFromCenter(u32 center, u32 primitive_restart_index, auto&& out_it) {
#if LIBRII_RINGITERATOR_DEBUG
    fmt::print(stderr, "Center: {} (valency={})\n", center, *max);
#endif
    // Break candidates up into islands sharing at least two vertices.
    TriangleFanSplitter splitter(mesh_, center);
    auto islands = splitter.ConvertToFans();

    std::vector<std::vector<u32>> fans;
    for (auto& island : islands) {
      // Only care about islands >= 4 in length (3 just becomes a strip)
      if (island.size() < min_fan_size_) {
        continue;
      }
      std::vector<u32> tmp(island.begin(), island.end());
      fans.push_back(tmp);
    }
    return fans;
  }

  std::span<const u32> mesh_{};
  int num_vertices_{};
  std::vector<bool> face_visited_{};

  // Holds twice the degree of each vertex N
  std::vector<int> valence_cache_{};
  int num_fans_{};

  // Options
  int min_fan_size_{};
};
template <typename OutputIteratorT, typename IndexTypeT>
bool TriFanPass::GenerateTriangleFansWithPrimitiveRestart(
    std::span<const u32> mesh, IndexTypeT primitive_restart_index,
    OutputIteratorT out_it, TriFanOptions options) {
  if (!Prepare(mesh, options)) {
    return false;
  }

  for (size_t i = 0; i < num_vertices_; ++i) {
    auto max = std::max_element(valence_cache_.begin(), valence_cache_.end());
    assert(max != valence_cache_.end());
    size_t center = max - valence_cache_.begin();
    // We've considered every vertex
    if (*max < min_fan_size_) {
      break;
    }
    // Effectively kill this vertex from our subgraph moving forward
    valence_cache_[center] = 0;
    auto fans = FindFansFromCenter(center, primitive_restart_index, out_it);
    for (auto& fan : fans) {
      StoreFan(center, fan, out_it);
      *out_it++ = primitive_restart_index;
    }
  }

  // Output remaining triangles in bulk
  for (size_t i = 0; i < face_visited_.size(); ++i) {
    if (!face_visited_[i]) {
      *out_it++ = mesh[i * 3];
      *out_it++ = mesh[i * 3 + 1];
      *out_it++ = mesh[i * 3 + 2];
      *out_it++ = primitive_restart_index;
    }
  }

  return true;
}

template <typename IndexTypeT>
coro::generator<std::span<const IndexTypeT>>
SplitByPrimitiveRestart(std::span<const IndexTypeT> mesh,
                        IndexTypeT primitive_restart_index) {
  size_t last = 0;
  for (size_t i = 0; i < mesh.size(); ++i) {
    if (mesh[i] == primitive_restart_index) {
      co_yield mesh.subspan(last, i - last);
      last = i + 1;
      continue;
    }
  }
}

Result<void> ToFanTriangles(MatrixPrimitive& prim) {
  auto buf = TRY(IndexBuffer<u32>::create(prim));

  TriFanPass fan_pass;
  std::vector<u32> fans;
  TriFanOptions options{};
  bool ok = fan_pass.GenerateTriangleFansWithPrimitiveRestart(
      buf.index_data, ~0u, std::back_inserter(fans), options);
  EXPECT(ok);

  Primitive triangles;
  triangles.topology = Topology::Triangles;
  for (auto fan_i : SplitByPrimitiveRestart<u32>(fans, ~0u)) {
    assert(fan_i.size() >= 3);
    auto fan_v = fan_i | std::views::transform([&](u32 idx) -> Vertex {
                   assert(idx != ~0u);
                   assert(idx < buf.vertices.size());
                   return buf.vertices[idx];
                 });
    if (fan_i.size() == 3) {
      triangles.vertices.insert(triangles.vertices.end(), fan_v.begin(),
                                fan_v.end());
    } else {
      auto& p = prim.primitives.emplace_back();
      p.topology = Topology::TriangleFan;
      p.vertices.insert(p.vertices.end(), fan_v.begin(), fan_v.end());
    }
  }
  // Initial triangles
  prim.primitives.erase(prim.primitives.begin());

  if (!triangles.vertices.empty()) {
    fprintf(stderr, "-> Running all other algos on remainder\n");
    MatrixPrimitive tmp;
    tmp.draw_matrices = prim.draw_matrices;
    tmp.primitives.push_back(triangles);
    TRY(StripifyTriangles(tmp, Algo::RiiFans));
    for (auto& x : tmp.primitives) {
      prim.primitives.push_back(x);
    }
  }
  fmt::print(stderr, ":: In summation, ");
  u32 score = 0;
  for (auto& p : prim.primitives) {
    score += p.vertices.size();
  }
  LogDone(buf.index_data.size(), score);
  return {};
}


struct DracoMesh {
  std::shared_ptr<draco::Mesh> mesh{};
  IndexBuffer<u32> index_manager{};
  size_t index_count{};
  size_t vertex_count{};
};

Result<DracoMesh> ToDraco(const MatrixPrimitive& prim) {
  auto buf = TRY(IndexBuffer<u32>::create(prim));
  auto& index_data = buf.index_data;
  auto& vertices = buf.vertices;
  assert(index_data.size() % 3 == 0);
  size_t index_count = index_data.size();
  size_t vertex_count = vertices.size();

  auto mesh = std::make_shared<draco::Mesh>();
  for (size_t i = 0; i < index_data.size(); i += 3) {
    std::array<draco::PointIndex, 3> face;
    face[0] = index_data[i];
    face[1] = index_data[i + 1];
    face[2] = index_data[i + 2];
    mesh->AddFace(face);
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
  mesh->SetAttribute(0, std::move(pos));
  mesh->SetAttribute(1, std::move(other));
  return DracoMesh{
      .mesh = std::move(mesh),
      .index_manager = std::move(buf),
      .index_count = index_count,
      .vertex_count = vertex_count,
  };
}

Result<void> StripifyTrianglesDraco(MatrixPrimitive& prim, bool degen) {
  auto draco_mesh = TRY(ToDraco(prim));
  auto& [mesh, buf, index_count, vertex_count] = draco_mesh;
  draco::MeshStripifier stripifier;
  std::vector<u32> strip;
  bool ok = false;
  if (degen) {
    ok = stripifier.GenerateTriangleStripsWithDegenerateTriangles(
        *mesh, std::back_inserter(strip));
  } else {
    ok = stripifier.GenerateTriangleStripsWithPrimitiveRestart(
        *mesh, ~0u, std::back_inserter(strip));
  }
  EXPECT(ok);
  u32 strip_size = strip.size();
  u32 real_size = 0;
  for (u32 i = 0; i < strip_size; ++i) {
    if (strip[i] != ~0u) {
      ++real_size;
    }
  }
  LogDone(index_count, real_size);

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
    EXPECT(idx < buf.vertices.size());
    p->vertices.push_back(buf.vertices[idx]);
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

Result<void> StripifyTrianglesAlgo(MatrixPrimitive& prim, Algo algo) {
  fmt::print(stderr, "[");
  fmt::print(stderr, fmt::fg(fmt::color::red), "{}",
             magic_enum::enum_name(algo));
  fmt::print(stderr, "]  ");
  rsl::Timer timer;
  switch (algo) {
  case Algo::MeshOptmzr:
    TRY(StripifyTrianglesMeshOptimizer(prim));
    break;
  case Algo::TriStripper:
    TRY(StripifyTrianglesTriStripper(prim));
    break;
  case Algo::NvTriStrip:
    TRY(StripifyTrianglesNvTriStripPort(prim));
    break;
  case Algo::Haroohie:
    TRY(StripifyTrianglesHaroohie(prim));
    break;
  case Algo::Draco:
    TRY(StripifyTrianglesDraco(prim, false));
    break;
  case Algo::DracoDegen:
    TRY(StripifyTrianglesDraco(prim, true));
    break;
  case Algo::RiiFans:
    TRY(ToFanTriangles(prim));
    break;
  }
  totalStrippingMs += timer.elapsed();
  u32 face = 0;
  for (auto& p : prim.primitives) {
    if (p.topology == Topology::Triangles) {
      face += p.vertices.size() / 3;
    } else if (p.topology == Topology::TriangleStrip) {
      face += p.vertices.size() - 2;
    }
  }
  fmt::print(stderr, " -> faces: {}\n", face);
  return {};
}

// Brute-force every algorithm
Result<void> StripifyTriangles(MatrixPrimitive& prim,
                               std::optional<Algo> except) {
  std::vector<MatrixPrimitive> results;
  for (auto e : magic_enum::enum_values<Algo>()) {
    MatrixPrimitive tmp = prim;
    if (!except || *except != e) {
      TRY(StripifyTrianglesAlgo(tmp, e));
    }
    results.push_back(std::move(tmp));
  }
  u32 best_score = std::numeric_limits<u32>::max();
  u32 best_index = 0;
  for (size_t i = 0; i < results.size(); ++i) {
    u32 score = VertexCount(results[i]);
    if (score < best_score) {
      best_score = score;
      best_index = i;
    }
  }
  std::vector<std::string_view> winners;
  for (u32 i = 0; i < results.size(); ++i) {
    u32 score = 0;
    for (auto& p : results[i].primitives) {
      score += p.vertices.size();
    }
    if (score == best_score) {
      winners.push_back(magic_enum::enum_name(static_cast<Algo>(i)));
    }
  }
  fmt::print(stderr, fmt::fg(fmt::color::red), "{}", rsl::join(winners, ", "));
  fmt::print(stderr, " won\n\n");

  prim = std::move(results[best_index]);
  return {};
}

static coro::generator<Result<size_t>> AsTrianglesIdx(const Primitive& prim) {
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
    for (size_t i = 0; i < prim.vertices.size(); i += 3) {
      co_yield i;
      co_yield i + 1;
      co_yield i + 2;
    }
    co_return;
  default:
    break;
  }
  co_yield std::unexpected("Unexpected primitive type");
}

coro::generator<Result<Vertex>>
AsTriangles(std::span<const Primitive> primitives) {
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

Result<MatrixPrimitive> TriangulateMPrim(const MatrixPrimitive& prim) {
  MatrixPrimitive result = prim;
  result.primitives.clear();
  auto& tris = result.primitives.emplace_back();
  tris.topology = Topology::Triangles;
  for (auto vert : AsTriangles(prim.primitives)) {
    tris.vertices.push_back(TRY(vert));
  }
  return result;
}
Result<Mesh> TriangulateMesh(const Mesh& mesh) {
  Mesh result = mesh;
  result.matrix_primitives.clear();
  for (auto& mprim : mesh.matrix_primitives) {
    result.matrix_primitives.push_back(TRY(TriangulateMPrim(mprim)));
  }
  return result;
}

} // namespace librii::rhst
