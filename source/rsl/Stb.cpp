#include "Stb.hpp"

#include <stb_image.h>

namespace rsl::stb {

using STB_DELETER = decltype([](u8* x) { stbi_image_free(x); });
using STB_PTR = std::unique_ptr<u8, STB_DELETER>;

Result<ImageResult> load_from_memory(std::span<const u8> view) {
  int width = 0;
  int height = 0;
  int channels = 0;
  STB_PTR data{stbi_load_from_memory(view.data(), view.size_bytes(), &width,
                                     &height, &channels, STBI_rgb_alpha)};
  if (data == nullptr) {
    return std::unexpected("stbi_load_from_memory failed");
  }

  int encoded_size = width * height * 4;

  return ImageResult{
      .data = {data.get(), data.get() + encoded_size},
      .width = width,
      .height = height,
      .channels = channels,
  };
}
Result<ImageResult> load(std::string_view path) {
  int width = 0;
  int height = 0;
  int channels = 0;
  std::string path_(path);
  STB_PTR data{
      stbi_load(path_.c_str(), &width, &height, &channels, STBI_rgb_alpha)};
  if (data == nullptr) {
    return std::unexpected("stbi_load failed");
  }

  int encoded_size = width * height * 4;

  return ImageResult{
      .data = {data.get(), data.get() + encoded_size},
      .width = width,
      .height = height,
      .channels = channels,
  };
}

} // namespace rsl::stb
