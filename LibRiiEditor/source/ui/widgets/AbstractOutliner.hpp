#pragma once

#include "core/SelectionManager.hpp"
#include <imgui/imgui.h>
#include <oishii/types.hxx>
#include <algorithm>

/*
struct AbstractOutlinerFolder
{
	using res_t = int;

	AbstractOutlinerFolder() = default;
	~AbstractOutlinerFolder() = default;


	struct Sampler
	{
		bool empty() const noexcept;
	};

	struct Filter
	{
		bool test(const std::string&) const;
	};


	//! @brief Return the number of resources in the source that pass the filter.
	//!
	int calcNumFiltered(const Sampler& sampler, const Filter* filter = nullptr) const noexcept
	{
		// If no data, empty
		if (sampler.empty())
			return 0;

		// If we don't have a filter, everything is included.
		if (!filter)
			return sampler.size();

		return std::count_if(
			sampler.begin(), sampler.end(),
			[](int i) {return i > 3;});
	}

	//! @brief Format the title in the "<header> (<number of resources>)" format.
	//!
	std::string formatTitle(const res_vector_t& vec, const ImGuiTextFilter* filter = nullptr) const noexcept
	{
		int numFiltered = calcNumFiltered(vec, filter);
		return TConfig::res_icon_plural() + "  " + TConfig::res_name_plural() + " (" + std::to_string(numFiltered) + ")";
	}

	void draw(const Sampler& sampler, SelectionManager& selMgr, Filter* filter = nullptr)
	{
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (!ImGui::TreeNode(formatTitle(vec, pFilter).c_str()))
			return;
	}
};
*/
