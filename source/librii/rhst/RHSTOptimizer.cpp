#include "RHSTOptimizer.hpp"
#include "IndexBuffer.hpp"
#include "MeshUtils.hpp"

#include <rsmeshopt/include/rsmeshopt.h>

#include <fmt/color.h>
#include <rsl/Ranges.hpp>
#include <thread>

// TODO: Bad, intrusive dependency
#include <rsmeshopt/src/MeshUtils.hpp>

#if !defined(__APPLE__)
#define throw
#endif
#include <fort/fort.hpp>
#if !defined(__APPLE__)
#undef throw
#endif

namespace librii::rhst {

// Respects winding order
static void NormalizeTriInplace(auto& tri) {
  size_t minPos = 0;
  for (size_t i = 1; i < 3; ++i) {
    if (tri[i] < tri[minPos]) {
      minPos = i;
    }
  }

  // Rotate the array so that the minimum element comes first
  std::rotate(std::begin(tri), std::begin(tri) + minPos, std::end(tri));
}

// Validates that an optimization pass did not damage the model itself.
// - Duplicates are allowed
// - Degenerates are stripped
class TriList {
public:
  Result<void> SetFromMPrim(const MatrixPrimitive& prim) {
    triangles_.reserve(20'000);

    auto verts = MeshUtils::AsTriangles(prim.primitives);

    std::vector<std::array<Vertex, 3>> tris;
    int ctr = 0;
    std::array<Vertex, 3> tmp;
    for (auto v : verts) {
      tmp[ctr] = TRY(v);
      ++ctr;
      if (ctr == 3) {
        ctr = 0;
        tris.push_back(tmp);
      }
    }
    for (auto& tri : tris) {
      // Discard degenerate triangles
      if (tri[0] == tri[1] || tri[0] == tri[2] || tri[1] == tri[2] ||
          tri[0] == tri[2]) {
        continue;
      }
      NormalizeTriInplace(tri);
      InsertTriangle(tri[0], tri[1], tri[2]);
    }

    std::sort(triangles_.begin(), triangles_.end());

    return {};
  }

  bool operator==(const TriList& rhs) const = default;

private:
  struct Tri {
    Vertex a{}, b{}, c{};
    std::partial_ordering operator<=>(const Tri& rhs) const {
      if (auto cmp = a <=> rhs.a; cmp != 0)
        return cmp;
      if (auto cmp = b <=> rhs.b; cmp != 0)
        return cmp;
      return c <=> rhs.c;
    }
    bool operator==(const Tri&) const = default;
  };

  // Insert a triangle into the list unsorted
  void InsertTriangle(const Vertex& a, const Vertex& b, const Vertex& c) {
    Tri tri{a, b, c};
    triangles_.push_back(tri);
  }

public:
  // Sorted, duplicates allowed
  std::vector<Tri> triangles_;
};

Result<void> ValidateMeshesEqualImpl(const TriList& ll, const TriList& rl) {
  if (ll.triangles_.size() != rl.triangles_.size()) {
    return std::unexpected(
        std::format("Number of triangles does not match (l: {}, r: {})",
                    ll.triangles_.size(), rl.triangles_.size()));
  }
  for (size_t i = 0; i < ll.triangles_.size(); ++i) {
    auto a = ll.triangles_[i];
    auto b = rl.triangles_[i];
    if (a != b) {
#ifndef NDEBUG
      [[maybe_unused]] auto test = a.a.position <=> b.a.position;
      [[maybe_unused]] bool ok_a = a.a == b.a;
      [[maybe_unused]] bool ok_b = a.b == b.b;
#endif
      [[maybe_unused]] bool ok_c = a.c == b.c;
      return std::unexpected(std::format("Mismatch at triangle {}/{}", i,
                                         ll.triangles_.size() - 1));
    }
  }
  return {};
}
Result<void> ValidateMeshesEqual(const MatrixPrimitive& l,
                                 const MatrixPrimitive& r) {
  TriList ll, rl;
  if (auto ok = ll.SetFromMPrim(l); !ok) {
    return std::unexpected("Failed to validate. Initial mprim is invalid: " +
                           ok.error());
  }
  if (auto ok = rl.SetFromMPrim(r); !ok) {
    return std::unexpected("Failed to validate. New mprim is invalid: " +
                           ok.error());
  }
  return ValidateMeshesEqualImpl(ll, rl);
}

// Instruments collection of Optimizer stats of a certain primitive encoding
// algorithm like triangle stripification.
class MeshOptimizerStatsCollector {
public:
  MeshOptimizerStatsCollector(const MatrixPrimitive& prim) {
    prim_ = &prim;
    stats_.before_indices = VertexCount(*prim_);
    stats_.before_faces = FaceCount(*prim_);
    timer_.reset();
#ifndef NDEBUG
    backup_ = *prim_;
#endif
  }

  // Ends the session and returns the results.
  MeshOptimizerStats End() {
    stats_.after_indices = VertexCount(*prim_);
    stats_.after_faces = FaceCount(*prim_);
    stats_.ms_elapsed = timer_.elapsed();
#ifndef NDEBUG
    auto ok = ValidateMeshesEqual(backup_, *prim_);
    // assert(ok);
#endif
    return stats_;
  }

  void SetComment(std::string&& comment) {
    stats_.comment = std::move(comment);
  }

#ifndef NDEBUG
  void Verify() {
    auto ok = ValidateMeshesEqual(backup_, *prim_);
    // assert(ok);
  }
#endif

private:
  const MatrixPrimitive* prim_{};
  MeshOptimizerStats stats_{};
  rsl::Timer timer_{};
#ifndef NDEBUG
  MatrixPrimitive backup_;
#endif
};

// Test bench for a variety of "experiments -- different ways to encode a set of
// Primitives. Experiments are scored by vertex count; only the winning
// experiment will be selected for actual output.
template <typename KeyT> class MeshOptimizerExperimentHolder {
public:
  MeshOptimizerExperimentHolder(const MatrixPrimitive& baseline)
      : baseline_(baseline) {}

  // Creates an experiment with the specified index |key| based on the baseline.
  MatrixPrimitive& CreateExperiment(KeyT key) {
    // unordered_map guarantees reference stability
    // operator[] constructs elements as necessary
    return (experiments_[key] = baseline_);
  }

  const MatrixPrimitive& GetExperiment(KeyT key) const {
    assert(experiments_.contains(key));
    return experiments_.at(key);
  }

  [[nodiscard]] Result<void> ValidateExperimentWithBaseline(KeyT key) const {
    assert(experiments_.contains(key));
    // Constructing a TriList is sufficiently expensive to warrant caching.
    if (!baselineList_) {
      TriList list;
      TRY(list.SetFromMPrim(baseline_));
      baselineList_ = std::move(list);
    }
    TriList ref;
    TRY(ref.SetFromMPrim(experiments_.at(key)));
    return ValidateMeshesEqualImpl(*baselineList_, ref);
  }

  [[nodiscard]] Result<void> ValidateAllWithBaseline() const {
    if (!baselineList_) {
      TriList list;
      TRY(list.SetFromMPrim(baseline_));
      baselineList_ = std::move(list);
    }
    for (auto& [key, experiment] : experiments_) {
      TriList ref;
      TRY(ref.SetFromMPrim(experiments_.at(key)));
      auto ok = ValidateMeshesEqualImpl(*baselineList_, ref);
      if (!ok) {
        return std::unexpected(
            std::format("({}) -> {}", static_cast<int>(key), ok.error()));
      }
    }
    return {};
  }

  void SetStats(KeyT key, MeshOptimizerStats stats) { stats_[key] = stats; }
  std::optional<MeshOptimizerStats> GetStats(KeyT key) const {
    if (!stats_.contains(key)) {
      return std::nullopt;
    }
    return stats_.at(key);
  }

  // Compute the experiments that yielded the best result (lowest vertex count)
  coro::generator<KeyT> CalcWinners() const {
    u32 best_score = std::numeric_limits<u32>::max();
    for (auto& [_, experiment] : experiments_) {
      u32 score = VertexCount(experiment);
      if (score < best_score) {
        best_score = score;
      }
    }
    for (auto& [key, experiment] : experiments_) {
      if (VertexCount(experiment) == best_score) {
        co_yield static_cast<KeyT>(key);
      }
    }
  }

  const MatrixPrimitive& GetFirstWinner() const {
    for (KeyT winner : CalcWinners()) {
      return experiments_.at(winner);
    }
    assert(!"No experiments were added");
    std::unreachable();
  }
  const KeyT GetFirstWinnerAlgo() const {
    for (KeyT winner : CalcWinners()) {
      return winner;
    }
    assert(!"No experiments were added");
    std::unreachable();
  }

  u32 GetBaselineScore() const { return VertexCount(baseline_); }
  u32 GetBaselineFaceCount() const { return FaceCount(baseline_); }

  struct Score {
    KeyT key{};
    u32 score{};
    std::optional<MeshOptimizerStats> stats{};
  };

  std::vector<Score> Scores() const {
    std::vector<Score> result;
    std::optional<MeshOptimizerStats> no_stats{};
    for (auto& [key, experiment] : experiments_) {
      auto s = Score{
          .key = static_cast<KeyT>(key),
          .score = static_cast<u32>(VertexCount(experiment)),
          .stats = stats_.contains(key)
                       ? std::optional<MeshOptimizerStats>{stats_.at(key)}
                       : no_stats,
      };
      result.push_back(s);
    }
    return result;
  }

private:
  // Const as baseLineList may be generated based on this within a const
  // function.
  const MatrixPrimitive baseline_{};
  // For validation. Mutable so ValidateExperimentWithBaseline can remain const.
  mutable std::optional<TriList> baselineList_;
  std::unordered_map<KeyT, MatrixPrimitive> experiments_{};
  std::unordered_map<KeyT, MeshOptimizerStats> stats_{};
};

// Get a thousands-separated string e.g. "10,000" from a number |x|.
std::string ToEnUsString(auto&& x) {
  auto str = std::to_string(x);
  const std::size_t decimal_pos = str.find('.');
  // If there's no decimal point, we want to start inserting commas three
  // characters from the right. If there is a decimal point, we want to start
  // inserting commas three characters to the left of the decimal point.
  std::ptrdiff_t insert_pos =
      (decimal_pos == std::string::npos)
          ? std::ssize(str) - 3
          : static_cast<std::ptrdiff_t>(decimal_pos) - 3;

  // Insert commas until we've processed the entire string
  while (insert_pos > 0) {
    str.insert(insert_pos, ",");
    insert_pos -= 3;
  }

  return str;
}

// Print the results of a `MeshOptimizerExperimentHolder` run as a formatted
// table.
template <typename KeyT>
[[nodiscard]] std::string
PrintScoresOfExperiment(const MeshOptimizerExperimentHolder<KeyT>& holder) {
  struct Score {
    std::string_view name;
    u32 score;
    std::optional<MeshOptimizerStats> stats{};
  };
  std::vector<Score> scores;
  for (auto&& x : holder.Scores()) {
    Score s;
    s.name = magic_enum::enum_name(x.key);
    s.score = x.score;
    s.stats = x.stats;
    scores.push_back(s);
  }
  Score before;
  before.name = "No Optimization";
  before.score = holder.GetBaselineScore();
  before.stats = MeshOptimizerStats{
      .after_faces = holder.GetBaselineFaceCount(),
  };
  scores.push_back(before);

  fort::char_table table;
  table << fort::header << "Rank"
        << "Algo"
        << "Number of vertices"
        << "Reduction"
        << "Execution Time"
        << "Number of faces (incl. degens)"
        << "Comment" << fort::endr;

  // Sort
  std::ranges::sort(scores,
                    [](auto&& l, auto&& r) { return l.score < r.score; });

  std::set<u32> ranks;
  for (auto [_, score, _2] : scores) {
    ranks.insert(score);
  }

  for (auto [name, score, stats] : scores) {
    auto rank = std::distance(ranks.begin(), ranks.find(score));
    std::string time = stats ? std::format("{}ms", stats->ms_elapsed) : "?";
    std::string faces = stats ? ToEnUsString(stats->after_faces) : "?";
    auto reduction =
        100.0f * (1.0f - static_cast<float>(score) /
                             static_cast<float>(holder.GetBaselineScore()));
    table << std::format("#{}", rank + 1);
    table << name;
    table << ToEnUsString(score);
    table << fmt::format("{}%", reduction);
    table << time;
    table << faces;
    table << std::string(stats ? stats->comment : std::string{});
    table << fort::endr;
  }

  return table.to_string();
}

// Class for managing index buffers of TriangleFan and TriangleStrip data. Joins
// all simple strips/fans (size=3) into a single TRIANGLES buffer at the end.
class PrimitiveRestartSplitter {
public:
  PrimitiveRestartSplitter(Topology topology, std::span<const Vertex> vertices,
                           u32 primitive_restart_index)
      : topology_(topology), vertices_(vertices),
        primitive_restart_index_(primitive_restart_index) {}

  // Reserve memory in the index buffer for faster OutputIterator use based on
  // an upper bound calculated from |triangle_indices_count|.
  void Reserve(u32 triangle_indices_count) {
    indices_.reserve(rsmeshopt::meshopt_stripifyBound_(triangle_indices_count));
  }

  // Construct an output iterator to the list of indices for consumption from
  // TriFanMeshOptimizer.
  auto OutputIterator() { return std::back_inserter(indices_); }

  // Move in an existing index buffer |indices| for use with MeshOptimizer.
  void SetIndices(std::vector<u32>&& indices) { indices_ = std::move(indices); }

  // Convert cached index buffer into a list of RHST primitives.
  coro::generator<Primitive> Primitives() const;

private:
  Topology topology_{};
  std::span<const Vertex> vertices_{};
  u32 primitive_restart_index_{~0u};
  std::vector<u32> indices_{};
};

coro::generator<Primitive> PrimitiveRestartSplitter::Primitives() const {
  Primitive triangles{};
  triangles.topology = Topology::Triangles;
  for (auto strip : rsmeshopt::MeshUtils::SplitByPrimitiveRestart<unsigned int>(
           indices_, primitive_restart_index_)) {
    assert(strip.size() >= 3);
    Primitive* p{};
    Primitive tmp;
    // Triangle Fans and Triangle Strips of length 3 are just triangles.
    if (strip.size() == 3) {
      p = &triangles;
    } else {
      p = &tmp;
      p->topology = topology_;
    }
    for (auto u : strip) {
      assert(u != primitive_restart_index_);
      assert(u < vertices_.size());
      p->vertices.push_back(vertices_[u]);
    }
    if (strip.size() > 3) {
      co_yield *p;
    }
  }
  if (!triangles.vertices.empty()) {
    co_yield triangles;
  }
}

Result<MeshOptimizerStats>
StripifyTrianglesRSMESHOPT(rsmeshopt::StripifyAlgo algo,
                           MatrixPrimitive& prim) {
  MeshOptimizerStatsCollector stats(prim);
  auto buf = TRY(IndexBuffer<u32>::create(prim));
  auto& index_data = buf.index_data;
  auto& vertices = buf.vertices;
  assert(index_data.size() % 3 == 0);

  std::vector<rsmeshopt::vec3> verts;
  for (auto v : vertices) {
    verts.push_back({v.position.x, v.position.y, v.position.z});
  }
  auto strip = TRY(rsmeshopt::DoStripifyAlgo_(algo, index_data, verts, ~0u));
  PrimitiveRestartSplitter splitter(Topology::TriangleStrip, vertices, ~0u);
  splitter.SetIndices(std::move(strip));
  prim.primitives.clear();
  for (Primitive& p : splitter.Primitives()) {
    prim.primitives.push_back(std::move(p));
  }

  return stats.End();
}

Result<MeshOptimizerStats>
StripifyTrianglesMeshOptimizer(MatrixPrimitive& prim) {
  return StripifyTrianglesRSMESHOPT(rsmeshopt::StripifyAlgo::MeshOpt, prim);
}
Result<MeshOptimizerStats> StripifyTrianglesTriStripper(MatrixPrimitive& prim) {
  return StripifyTrianglesRSMESHOPT(rsmeshopt::StripifyAlgo::TriStripper, prim);
}
Result<MeshOptimizerStats>
StripifyTrianglesNvTriStripPort(MatrixPrimitive& prim) {
  return StripifyTrianglesRSMESHOPT(rsmeshopt::StripifyAlgo::NvTriStripPort,
                                    prim);
}

Result<MeshOptimizerStats> StripifyTrianglesHaroohie(MatrixPrimitive& prim) {
  return StripifyTrianglesRSMESHOPT(rsmeshopt::StripifyAlgo::Haroohie, prim);
}

Result<MeshOptimizerStats> ToFanTriangles(MatrixPrimitive& prim, u32 min_len,
                                          size_t max_runs) {
  MeshOptimizerStatsCollector stats(prim);
  auto buf = TRY(IndexBuffer<u32>::create(prim));

  auto fans = TRY(rsmeshopt::MakeFans_(buf.index_data, ~0u, min_len, max_runs));

  PrimitiveRestartSplitter splitter(Topology::TriangleFan, buf.vertices, ~0u);
  splitter.Reserve(buf.index_data.size());

  std::copy(fans.begin(), fans.end(), splitter.OutputIterator());

  prim.primitives.clear();
  for (Primitive& p : splitter.Primitives()) {
    prim.primitives.push_back(std::move(p));
  }
#ifndef NDEBUG
  stats.Verify();
#endif
  // PrimitiveRestartSplitter puts a batch of triangles at the very end if
  // there remain any.
  if (prim.primitives.size() > 0) {
    auto& triangles = prim.primitives[prim.primitives.size() - 1];
    if (triangles.topology == Topology::Triangles) {
      MatrixPrimitive tmp;
      tmp.draw_matrices = prim.draw_matrices;
      tmp.primitives.push_back(triangles);
      auto algo = TRY(StripifyTriangles(tmp, Algo::RiiFans));
      prim.primitives.resize(prim.primitives.size() - 1);
      for (auto& x : tmp.primitives) {
        prim.primitives.push_back(x);
      }
      stats.SetComment(std::format("min_len: {}, max_runs: {}, stripifier: {}",
                                   min_len, max_runs,
                                   magic_enum::enum_name(algo)));
    }
  }
  return stats.End();
}

Result<MeshOptimizerStats> ToFanTriangles2(MatrixPrimitive& prim) {
  MeshOptimizerStatsCollector stats(prim);
  auto vc = VertexCount(prim);
  if (vc >= 20'000) {
    // TODO: Workaround -- skips on sufficiently complex meshes, for now
    stats.SetComment("Skipping mesh to save time: too complex");
    return stats.End();
  }

  std::array<size_t, 6> depths = {vc, 5, 10, 20, 40, 80};

  MeshOptimizerExperimentHolder<size_t> experiments(prim);
  for (auto& d : depths) {
    auto& tmp = experiments.CreateExperiment(d);
    auto stats = TRY(ToFanTriangles(tmp, 4, d));
    experiments.SetStats(d, stats);
  }
  // TRY(experiments.ValidateAllWithBaseline());
  prim = experiments.GetFirstWinner();
  stats.SetComment(experiments.GetStats(experiments.GetFirstWinnerAlgo())
                       .value_or(MeshOptimizerStats{})
                       .comment);
  return stats.End();
}

Result<MeshOptimizerStats> StripifyTrianglesDraco(MatrixPrimitive& prim,
                                                  bool degen) {
  if (degen) {
    return StripifyTrianglesRSMESHOPT(rsmeshopt::StripifyAlgo::DracoDegen,
                                      prim);
  }
  return StripifyTrianglesRSMESHOPT(rsmeshopt::StripifyAlgo::Draco, prim);
}

Result<MeshOptimizerStats> StripifyTrianglesAlgo(MatrixPrimitive& prim,
                                                 Algo algo) {
  switch (algo) {
  case Algo::MeshOptmzr:
    return StripifyTrianglesMeshOptimizer(prim);
  case Algo::TriStripper:
    return StripifyTrianglesTriStripper(prim);
  case Algo::NvTriStrip:
    return StripifyTrianglesNvTriStripPort(prim);
  case Algo::Haroohie:
    return StripifyTrianglesHaroohie(prim);
  case Algo::Draco:
    return StripifyTrianglesDraco(prim, false);
  case Algo::DracoDegen:
    return StripifyTrianglesDraco(prim, true);
  case Algo::RiiFans:
    // This calls everything else on result.
    return ToFanTriangles2(prim);
  }
  return std::unexpected("Invalid mesh algorithm");
}

// Brute-force every algorithm
Result<Algo> StripifyTriangles(MatrixPrimitive& prim,
                               std::optional<Algo> except,
                               std::string_view debug_name, bool verbose) {
  MeshOptimizerExperimentHolder<Algo> experiments(prim);
  u32 ms_on_validate = 0;
  for (auto e : magic_enum::enum_values<Algo>()) {
    if (except && *except == e) {
      // Disabled by user input
      continue;
    }
    if (e == Algo::Haroohie) {
      // Disabled for now: The slowest but not with the best results to show.
      continue;
    }
    if (e == Algo::DracoDegen) {
      // Disabled for now. Unusable output.
      continue;
    }
    if (e == Algo::TriStripper) {
      // This almost *never* wins, and is quite slow at that, but is here so we
      // can never possibly lose to BrawlBox.
    }
    auto& tmp = experiments.CreateExperiment(e);
    auto results = StripifyTrianglesAlgo(tmp, e);
    if (!results) {
      experiments.CreateExperiment(e);
      experiments.SetStats(e, {.comment = results.error()});
      continue;
    }
    // If invalid, reset and comment the error
    rsl::Timer timer;
    auto ok = experiments.ValidateExperimentWithBaseline(e);
    ms_on_validate += timer.elapsed();
    if (!ok) {
      experiments.CreateExperiment(e);
      experiments.SetStats(e, {.comment = ok.error()});
      continue;
    }
    experiments.SetStats(e, *results);
  }
  std::vector<std::string_view> winners;
  for (Algo e : experiments.CalcWinners()) {
    winners.push_back(magic_enum::enum_name(e));
  }
  if (verbose && !except) {
    auto table = PrintScoresOfExperiment(experiments);
    std::stringstream thread_id;
#ifndef __APPLE__
    thread_id << std::this_thread::get_id();
#endif
    fmt::print(stderr,
               "----\n| Compiling {} on thread {}\n{}| Spent {}ms on "
               "validation\n---\n",
               debug_name, thread_id.str(), table, ms_on_validate);
  }
  prim = experiments.GetFirstWinner();
  return experiments.GetFirstWinnerAlgo();
}

} // namespace librii::rhst
