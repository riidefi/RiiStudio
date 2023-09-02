#ifndef RSMESHOPT_H
#define RSMESHOPT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

uint32_t rii_stripify(uint32_t* dst, uint32_t algo, const uint32_t* indices,
                      uint32_t num_indices, const float* positions,
                      uint32_t num_positions, uint32_t restart);

uint32_t rii_makefans(uint32_t* dst, const uint32_t* indices,
                      uint32_t num_indices, uint32_t restart, uint32_t min_len,
                      uint32_t max_runs);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#ifndef RSMESHOPT_NO_EXPECTED
#include <expected>
#endif
#include <span>
#include <string>
#include <vector>

namespace rsmeshopt {

static inline uint32_t meshopt_stripifyBound_(uint32_t x) {
  return (x / 3) * 5;
}

struct vec3 {
  float x, y, z;
};

enum class StripifyAlgo {
  NvTriStripPort = 0,
  TriStripper,
  MeshOpt,
  Draco,
  DracoDegen,
  Haroohie
};

static inline std::expected<std::vector<uint32_t>, std::string>
DoStripifyAlgo_(StripifyAlgo algo, std::span<const uint32_t> index_data,
                std::span<const vec3> vertex_data, uint32_t restart = ~0u) {
  std::vector<uint32_t> dst(meshopt_stripifyBound_(index_data.size()));

  uint32_t result_size =
      ::rii_stripify(dst.data(), static_cast<uint32_t>(algo), index_data.data(),
                     static_cast<uint32_t>(index_data.size()),
                     reinterpret_cast<const float*>(vertex_data.data()),
                     static_cast<uint32_t>(vertex_data.size()), restart);

  if (result_size == 0) {
    return std::unexpected("Failed to stripify.");
  }

  dst.resize(result_size);
  return dst;
}

static inline std::expected<std::vector<uint32_t>, std::string>
MakeFans_(std::span<const uint32_t> index_data, uint32_t restart,
          uint32_t min_len, uint32_t max_runs) {
  // Unless our algorithm goes really wrong, this should still be the upper
  // bound
  std::vector<uint32_t> dst(meshopt_stripifyBound_(index_data.size()));
  uint32_t result_size = ::rii_makefans(
      dst.data(), index_data.data(), static_cast<uint32_t>(index_data.size()),
      restart, min_len, max_runs);

  if (result_size == 0) {
    return std::unexpected("Failed to make fans.");
  }

  dst.resize(result_size);
  return dst;
}

} // namespace rsmeshopt
#endif

#endif /* RSMESHOPT_H */
