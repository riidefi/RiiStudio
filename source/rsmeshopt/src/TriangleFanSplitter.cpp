#include "TriangleFanSplitter.hpp"

#include <array>
#include <unordered_map>

namespace rsmeshopt {

std::vector<std::set<size_t>>
TriangleFanSplitter::ConvertToFans(std::span<const u32> mesh, u32 center) {
  mesh_ = mesh;
  center_ = center;
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
    std::vector<std::set<size_t>*> its;
    for (auto& island : islands_) {
      if (CanAddToFan(island, cand_face_indices)) {
        its.push_back(&island);
      }
    }

    if (its.empty()) {
      // No existing island: create a new one
      std::set<size_t> island;
      island.insert(cand_face);
      islands_.push_back(island);
    } else if (its.size() == 2) {
      // Merge the two
      its[0]->insert(cand_face);
      for (size_t f : *its[1]) {
        its[0]->insert(f);
      }
      std::erase(islands_, *its[1]);
    } else {
      // It's fairly unlikely, but possible, the triangle could connect to
      // multiple fans. For now, just don't bother.
      its[0]->insert(cand_face);
    }
  }

  return islands_;
}

bool TriangleFanSplitter::CanAddToFan(const std::set<size_t>& island,
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
                       cand_vert, island_edge_from, island_vert, island_edge_to,
                       cand_edge_from, cand_vert, cand_edge_to);
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

} // namespace rsmeshopt
