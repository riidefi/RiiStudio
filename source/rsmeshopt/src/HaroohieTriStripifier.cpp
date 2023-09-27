// Based on jelle/Gericom's implementation

#include "HaroohieTriStripifier.hpp"
#include <limits>
#include <string>
#include <tuple>

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

std::vector<std::vector<int>>
TriangleStripifier::implTriangleStripify(std::span<const int> indices) {
  assert(indices.size() % 3 == 0);
  std::vector<Primitive> primitives;
  for (size_t i = 0; i < indices.size(); i += 3) {
    Primitive prim{};
    prim.vertex_indices.push_back(static_cast<int>(indices[i]));
    prim.vertex_indices.push_back(static_cast<int>(indices[i + 1]));
    prim.vertex_indices.push_back(static_cast<int>(indices[i + 2]));
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
