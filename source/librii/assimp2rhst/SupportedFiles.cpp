#include "SupportedFiles.hpp"
#include <core/common.h>

#include <algorithm>

namespace librii::assimp2rhst {

static constexpr std::array<std::string_view, 4> supported_endings = {
    ".dae", ".obj", ".fbx", ".smd"};

std::string to_lower(std::string_view file) {
  std::string lower_file(file);
  for (auto& c : lower_file)
    c = std::tolower(c);

  return lower_file;
}

bool IsExtensionSupported(std::string_view path) {
  const auto lower_file = to_lower(path);

  return std::any_of(supported_endings.begin(), supported_endings.end(),
                     [&](const std::string_view& ending) {
                       return lower_file.ends_with(ending);
                     });
}

} // namespace librii::assimp2rhst
