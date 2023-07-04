#pragma once

#include <expected>
#include <span>
#include <stdint.h>
#include <string>
#include <vector>

#include <glm/vec3.hpp>

#include "TriStripper/public_types.h"

namespace rsmeshopt {

std::expected<std::vector<std::vector<uint32_t>>, std::string>
StripifyTrianglesNvTriStripPort(std::span<const uint32_t> index_data);

std::expected<triangle_stripper::primitive_vector, std::string>
StripifyTrianglesTriStripper(std::span<const uint32_t> index_data);

// From meshoptimizer
size_t meshopt_stripify(unsigned int* destination, const unsigned int* indices,
                        size_t index_count, size_t vertex_count,
                        unsigned int restart_index);
size_t meshopt_stripifyBound(size_t index_count);
size_t meshopt_unstripify(unsigned int* destination,
                          const unsigned int* indices, size_t index_count,
                          unsigned int restart_index);
size_t meshopt_unstripifyBound(size_t index_count);

std::expected<std::vector<u32>, std::string>
StripifyDraco(std::span<u32> index_data, std::span<glm::vec3> vertex_data,
              u32 restart = ~0u, bool degen = false);

} // namespace rsmeshopt
