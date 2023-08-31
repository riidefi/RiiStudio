#pragma once

#include <rsl/Expected.hpp>

#include <span>
#include <stdint.h>
#include <string>
#include <vector>

#include <glm/vec3.hpp>

#include "TriStripper/public_types.h"

namespace rsmeshopt {

std::expected<std::vector<std::vector<uint32_t>>, std::string>
StripifyTrianglesNvTriStripPort(std::span<const uint32_t> index_data);

std::expected<std::vector<u32>, std::string>
StripifyTrianglesNvTriStripPort2(std::span<const u32> index_data, u32 restart);

std::expected<triangle_stripper::primitive_vector, std::string>
StripifyTrianglesTriStripper(std::span<const uint32_t> index_data);

std::expected<std::vector<u32>, std::string>
StripifyTrianglesTriStripper2(std::span<const u32> index_data, u32 restart);

// From meshoptimizer
size_t meshopt_stripify(unsigned int* destination, const unsigned int* indices,
                        size_t index_count, size_t vertex_count,
                        unsigned int restart_index);
size_t meshopt_stripifyBound(size_t index_count);
size_t meshopt_unstripify(unsigned int* destination,
                          const unsigned int* indices, size_t index_count,
                          unsigned int restart_index);
size_t meshopt_unstripifyBound(size_t index_count);

std::vector<u32> StripifyMeshOpt(std::span<const u32> index_data,
                                 u32 vertex_count, // For debug checks
                                 u32 restart = ~0u);

std::expected<std::vector<u32>, std::string>
StripifyDraco(std::span<const u32> index_data,
              std::span<const glm::vec3> vertex_data, u32 restart = ~0u,
              bool degen = false);

std::expected<std::vector<u32>, std::string>
StripifyHaroohie(std::span<const u32> index_data, u32 restart = ~0u);

enum class StripifyAlgo {
  NvTriStripPort,
  TriStripper,
  MeshOpt,
  Draco, // degen = false
  Haroohie,
};
std::expected<std::vector<u32>, std::string>
DoStripifyAlgo(StripifyAlgo algo, std::span<const u32> index_data,
             std::span<const glm::vec3> vertex_data, u32 restart = ~0u);

std::expected<std::vector<u32>, std::string>
MakeFans(std::span<const u32> index_data, u32 restart, u32 min_len, u32 max_runs);

} // namespace rsmeshopt
