#include "SupportedFiles.hpp"

#include <array>
#include <core/common.h>
#include <string>

#include <algorithm>

#ifdef HAS_RANGES
#include <ranges>
#endif

#include <locale> // tolower

namespace riistudio::ass {

static constexpr std::array<std::string_view, 4> supported_endings = {
    ".dae", ".obj", ".fbx", ".smd"};

std::string to_lower(std::string_view file) {
  std::string lower_file(file);
  for (auto& c : lower_file)
    c = tolower(c);

  return lower_file;
}

bool IsExtensionSupported(std::string_view path) {
  const auto lower_file = to_lower(path);

#ifdef HAS_RANGES
  return std::ranges::any_of(supported_endings,
                             [&](const std::string_view& ending) {
                               return lower_file.ends_with(ending);
                             });
#else
  return std::any_of(supported_endings.begin(), supported_endings.end(),
                     [&](const std::string_view& ending) {
                       return lower_file.ends_with(ending);
                     });
#endif
}

} // namespace riistudio::ass
