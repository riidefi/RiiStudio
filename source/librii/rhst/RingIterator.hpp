#pragma once

#include <core/common.h>
#include <fmt/core.h>

namespace librii::rhst {

#define LIBRII_RINGITERATOR_DEBUG 0

// Returns true if two edges |a| and |b| are connected.
inline bool EdgeCompare(auto&& l, auto&& r) {
  return l.first == r.first || l.first == r.second || l.second == r.first ||
         l.second == r.second;
};

// Given some vertex |center|, and all the triangles have a vertex at |center|
// and are connected to each other, iterate over the non-|center| vertices in
// connected order. That is, each next vertex RingIterator produces will be
// connected to the last vertex it produced. It's important the RingIterator
// starts at the right vertex; there are two options, the start and end.
template <typename IndexTypeT = size_t> class RingIterator {
public:
  // Triangles provided as a list of vertices. This length of this array must be
  // a multiple of three.
  RingIterator(IndexTypeT center, std::span<const IndexTypeT> indices)
      : center_(center) {
#if LIBRII_RINGITERATOR_DEBUG
    for (size_t i = 0; i < indices.size(); i += 3) {
      fmt::print(stderr, "{}->{};\n", indices[i], indices[i + 1]);
      fmt::print(stderr, "{}->{};\n", indices[i + 1], indices[i + 2]);
      fmt::print(stderr, "{}->{};\n", indices[i + 2], indices[i]);
    }
    fmt::print(stderr, " aboveDump\n");
#endif
    for (size_t i = 0; i < indices.size(); i += 3) {
      // Ensure each triangle shares |center| exactly once
      if (std::ranges::count(indices.subspan(i, 3), center_) != 1) {
        valid_ = false;
        return;
      }
      // Find the two non-center vertices of each triangle
      ptrdiff_t v1 = -1, v2 = -1;
      for (size_t j = 0; j < 3; j++) {
        if (indices[i + j] != center_ && indices[i + (j + 1) % 3] != center_) {
          v1 = static_cast<ptrdiff_t>(indices[i + j]);
          v2 = static_cast<ptrdiff_t>(indices[i + (j + 1) % 3]);
          break;
        }
      }
      assert(v1 != -1 && v2 != -1);
      // Create the edge
      auto edge = std::make_pair<size_t, size_t>(v1, v2);
      // Ignore duplicate edges
      auto it = std::ranges::find(edges_, edge);
      if (it == edges_.end()) {
        edges_.push_back(edge);
      }
    }
    sortEdges();
#if LIBRII_RINGITERATOR_DEBUG
    for (size_t i = 0; i < edges_.size(); ++i) {
      fmt::print(stderr, "edge {}: ({}, {})\n", i, edges_[i].first,
                 edges_[i].second);
    }
#endif
    for (size_t i = 1; i < edges_.size(); ++i) {
      assert(EdgeCompare(edges_[i - 1], edges_[i]));
    }

    current_edge_ = edges_.begin();
    last_vertex_ = -1;
    valid_ = true;
  }

  // Validates that:
  // 1) All triangles share one vertex, the |center|.
  // 2) All triangles are connected
  bool valid() const { return valid_; }

  // Returns nullopt if exhausted.
  std::optional<IndexTypeT> nextVertex() {
    if (!valid_ || current_edge_ == edges_.end()) {
      return std::nullopt;
    }
    auto next_vertex = current_edge_->first == last_vertex_
                           ? current_edge_->second
                           : current_edge_->first;
    last_vertex_ = next_vertex;
    ++current_edge_;
    return next_vertex;
  }

  struct Sentinel {};

  class Iterator {
  public:
    Iterator(RingIterator& ringIterator) : ringIterator_(&ringIterator) {
      current_ = ringIterator_->nextVertex();
    }
    IndexTypeT operator*() {
      assert(current_.has_value());
      return current_.value();
    }
    bool operator!=(const Sentinel&) const { return current_.has_value(); }
    Iterator& operator++() {
      current_ = ringIterator_->nextVertex();
      return *this;
    }

  private:
    RingIterator* ringIterator_;
    std::optional<IndexTypeT> current_;
  };
  Iterator begin() { return Iterator(*this); }
  Sentinel end() { return Sentinel{}; }

private:
  void sortEdges() {
    std::unordered_map<int, int> valency;
    std::unordered_map<int, std::vector<int>> from_to;
    for (auto& [l, r] : edges_) {
      ++valency[l];
      ++valency[r];
      from_to[l].push_back(r);
    }
#if LIBRII_RINGITERATOR_DEBUG
    for (auto& [v, n] : edges_) {
      fmt::print(stderr, "{}->{};\n", v, n);
    }
    for (auto& [v, n] : valency) {
      fmt::print(stderr, "Vert: {}, Valency: {}\n", v, n);
    }
#endif
    // Pick the right start
    auto valency_tmp = valency;
    std::erase_if(valency_tmp,
                  [&](auto& v) { return from_to[v.first].empty(); });
    auto start = std::ranges::min_element(
        valency_tmp, [&](auto& l, auto& r) { return l.second < r.second; });
    assert(start->second <= 4 && "edges_ likely contains duplicates");
    // In case of 2, it's a cycle
    std::vector<EdgeT> tmp;
    int cur = start->first;
#if LIBRII_RINGITERATOR_DEBUG
    fmt::print(stderr, "Starting with vert {}\n", cur);
#endif
    for (int i = 0; i < edges_.size(); ++i) {
      auto cands = from_to[cur];
      if (cands.size() == 0) {
        assert(i + 1 == edges_.size() && "Not at end of cycle");
        break;
      }
      // If this fails, we inserted a triangle with bad winding order.
      assert(cands.size() >= 1);
      auto next = cands[0];
      // We attached a triangle with invalid winding order.
      assert(next != cur);
      tmp.push_back({cur, next});
      cur = next;
    }
    tmp.push_back({cur, cur});
    edges_ = tmp;
  }

  IndexTypeT center_;
  bool valid_;
  using EdgeT = std::pair<IndexTypeT, IndexTypeT>;
  std::vector<EdgeT> edges_;
  typename std::vector<EdgeT>::iterator current_edge_;
  IndexTypeT last_vertex_;
};

} // namespace librii::rhst
