#pragma once

#include <string_view>

namespace kpi {

struct RichIcon {
  std::string_view icon_plural = "(?)";
  std::string_view icon_singular = "(?)";
};
struct RichName {
  std::string_view exposedName;
  std::string_view namespacedId;
  std::string_view commandName;

  RichIcon icon{};
  //	RichName(std::string_view exposed, std::string_view namespaced,
  // std::string_view command) 		: exposedName(exposed),
  // namespacedId(namespaced), commandName(command)
  //	{}
};

} // namespace kpi
