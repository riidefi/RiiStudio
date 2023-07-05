#pragma once

#include <core/common.h>

#include <librii/rhst/RHST.hpp>

namespace librii::rhst {

struct MeshOptimizerStats {
  // Number of "facepoints"
  u32 before_indices{};
  u32 after_indices{};

  // This includes degenerates
  u32 before_faces{};
  u32 after_faces{};

  // For this algorithm
  u32 ms_elapsed{};

  std::string comment;
};

// Uses zeux/meshoptimizer
Result<MeshOptimizerStats>
StripifyTrianglesMeshOptimizer(MatrixPrimitive& prim);

// Uses GPSnoopy/TriStripper
Result<MeshOptimizerStats> StripifyTrianglesTriStripper(MatrixPrimitive& prim);

// Uses amorilia/tristrip's port of NVTriStrip w/o cache support (fine, our
// target doesn't have a post-TnL cache)
// - Removed use of boost
// - Replaced throw() with assertions
Result<MeshOptimizerStats>
StripifyTrianglesNvTriStripPort(MatrixPrimitive& prim);

// C++ port of jellees/nns-blender-plugin
Result<MeshOptimizerStats> StripifyTrianglesHaroohie(MatrixPrimitive& prim);

Result<MeshOptimizerStats> StripifyTrianglesDraco(MatrixPrimitive& prim,
                                                  bool allow_degen);

Result<MeshOptimizerStats>
ToFanTriangles(MatrixPrimitive& prim, u32 min_len = 4,
               size_t max_runs = std::numeric_limits<size_t>::max());

enum class Algo {
  NvTriStrip,
  Draco,
  Haroohie,
  TriStripper,
  MeshOptmzr,
  DracoDegen,
  RiiFans,
};
Result<MeshOptimizerStats> StripifyTrianglesAlgo(MatrixPrimitive& prim,
                                                 Algo algo);

// Brute-force every algorithm
Result<Algo> StripifyTriangles(MatrixPrimitive& prim,
                               std::optional<Algo> except = std::nullopt,
                               std::string_view debug_name = "?",
                               bool verbose = true);

} // namespace librii::rhst
