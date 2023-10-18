// See
// https://github.com/riidefi/RiiStudio/blob/master/source/rsmeshopt/include/rsmeshopt.h
//
// Main APIs:
//
// std::expected<std::vector<uint32_t>, std::string>
// DoStripifyAlgo_(StripifyAlgo algo, std::span<const uint32_t> index_data,
//                std::span<const vec3> vertex_data, uint32_t restart = ~0u);
//
#include "rsmeshopt.h"

#include <array>
#include <iostream>
#include <vector>

int main() {
  std::string version = rsmeshopt::get_version();
  std::cout << "rsmeshopt version: " << version << std::endl;

  // 0---1
  // |  /|
  // | / |
  // 2---3

  std::array<uint32_t, 6> square{
      2, 1, 0, // Triangle 1
      1, 2, 3, // Triangle 2
  };

  // Triangle strip
  auto stripped_maybe = rsmeshopt::DoStripifyAlgo_(
      rsmeshopt::StripifyAlgo::NvTriStripPort, square, {});
  if (!stripped_maybe) {
    std::cout << "Failed to strip: " << stripped_maybe.error() << std::endl;
    return -1;
  }
  std::vector<uint32_t> stripped = *stripped_maybe;
  std::cout << "Stripped: ";
  for (auto val : stripped) {
    std::cout << val << " ";
  }
  std::cout << std::endl;

  return 0;
}
