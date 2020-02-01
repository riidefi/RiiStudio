#pragma once

#include <string_view>

namespace px {

struct RichName
{
	std::string_view exposedName;
	std::string_view namespacedId;
	std::string_view commandName;
};

} // namespace pl
