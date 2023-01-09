#pragma once

#include <core/common.h>

namespace rsl::stb {

struct ImageResult {
  std::vector<u8> data;
  int width{};
  int height{};
  int channels{};
};

[[nodiscard]] Result<ImageResult> load_from_memory(std::span<const u8> view);
[[nodiscard]] Result<ImageResult> load(std::string_view path);

} // namespace rsl::stb
