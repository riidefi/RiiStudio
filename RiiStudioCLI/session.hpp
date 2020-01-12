#pragma once

#include <string>
#include <vector>
#include <string_view>
#include <LibRiiEditor/pluginapi/Plugin.hpp>

namespace rs_cli {

class System;

class Session
{
public:

	struct Node
	{
		std::string mFormatId;
		std::string mPath;
		bool mPathImplicit;
		std::vector<std::string> mOptions;
	};

	void populate(std::vector<std::string_view> args);
	void execute(System& sys);

private:
	std::vector<Node> mNodes;
};


} // namespace rs_cli
