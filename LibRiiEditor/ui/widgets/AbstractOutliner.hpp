#pragma once

#include "LibRiiEditor/core/SelectionManager.hpp"
#include <imgui/imgui.h>
#include <oishii/types.hxx>
#include <algorithm>
#include <string>


/*
Filter interface:

struct Filter
{
	bool test(const std::string&) const noexcept;
};

Sampler interface:

struct Sampler
{
	bool empty() const noexcept;
	u32 size() const noexcept;
	const char* nameAt(u32 idx) const noexcept;
	void* rawAt(u32 idx) const noexcept; // For selection
};
*/
struct AbstractOutlinerConfigDefault
{
	static std::string res_name_plural()
	{
		return "Unknowns";
	}
	static std::string res_name_singular()
	{
		return "Unknown";
	}
	static std::string res_icon_plural()
	{
		return "(?)";
	}
	static std::string res_icon_singular()
	{
		return "(?)";
	}
};

struct ImTFilter : public ImGuiTextFilter
{
	bool test(const std::string& str) const noexcept
	{
		return PassFilter(str.c_str());
	}
};

template<typename T>
struct VectorSampler
{
	bool empty() const noexcept
	{
		return data.empty();
	}
	u32 size() const noexcept
	{
		return data.size();
	}
	std::string nameAt(u32 idx) const noexcept
	{
		return data[idx].getName();
	}
	void* rawAt(u32 idx) const noexcept
	{
		return &data[idx];
	}
	std::vector<T>& data;
};

template<class TSampler, SelectionManager::Type ESelection = SelectionManager::Type::Texture, typename TConfig = AbstractOutlinerConfigDefault, class TFilter = ImTFilter>
struct AbstractOutlinerFolder
{
	AbstractOutlinerFolder() = default;
	~AbstractOutlinerFolder() = default;


	//! @brief Return the number of resources in the source that pass the filter.
	//!
	int calcNumFiltered(const TSampler& sampler, const TFilter* filter = nullptr) const noexcept
	{
		// If no data, empty
		if (sampler.empty())
			return 0;

		// If we don't have a filter, everything is included.
		if (!filter)
			return sampler.size();

		u32 nPass = 0;

		for (u32 i = 0; i < sampler.size(); ++i)
			if (filter->test(sampler.nameAt(i).c_str()))
				++nPass;

		return nPass;
	}

	//! @brief Format the title in the "<header> (<number of resources>)" format.
	//!
	std::string formatTitle(const TSampler& sampler, const TFilter* filter = nullptr) const noexcept
	{
		return TConfig::res_icon_plural() + "  " + TConfig::res_name_plural() + " (" + std::to_string(calcNumFiltered(sampler, filter)) + ")";
	}

	void draw(const TSampler& sampler, SelectionManager& selMgr, TFilter* filter = nullptr)
	{
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (!ImGui::TreeNode(formatTitle(sampler, filter).c_str()))
			return;

		// A filter tree for multi selection. Prevents inclusion of unfiltered data with SHIFT clicks.
		std::vector<int> filtered;

		int justSelectedId = -1;
		// Relative to filter vector.
		int justSelectedFilteredIdx = -1;
		// Necessary to filter out clicks on already selected items.
		bool justSelectedAlreadySelected = false;
		// Prevent resetting when SHIFT is unpressed with arrow keys.
		bool thereWasAClick = false;

		// Draw the tree
		for (int i = 0; i < sampler.size(); ++i)
		{
			auto n = sampler.nameAt(i);
			const char* name = n.c_str();
			if (filter && !filter->test(name))
				continue;

			filtered.push_back(i);

			// Whether or not this node is already selected.
			// Selections from other windows will carry over.
			bool curNodeSelected = selMgr.isSelected(ESelection, sampler.rawAt(i));

			bool bSelected = ImGui::Selectable(std::to_string(i).c_str(), curNodeSelected);
			ImGui::SameLine();

			thereWasAClick = ImGui::IsItemClicked();
			bool focused = ImGui::IsItemFocused();

			const std::string cur_name = TConfig::res_icon_singular() + " " + std::string(name);

			if (ImGui::TreeNodeEx(std::to_string(i).c_str(), ImGuiTreeNodeFlags_Leaf, cur_name.c_str()))
			{
				// NodeDrawer::drawNode(*node.get());

				// If there waas a click above, we need to ignore the focus below.
				// Assume only one item can be clicked.
				if (thereWasAClick || (focused && justSelectedId == -1 && !curNodeSelected))
				{
					// If it was already selected, we may need to reprocess for ctrl clicks followed by shift clicks
					justSelectedAlreadySelected = curNodeSelected;
					justSelectedId = i;
					justSelectedFilteredIdx = filtered.size() - 1;
				}

				ImGui::TreePop();
			}
		}
		ImGui::TreePop();

		// If nothing new was selected, no new processing needs to occur.
		if (justSelectedId == -1)
			return;

		// Currently, nothing for ctrl + shift or ctrl + art.
		// If both are pressed, SHIFT takes priority.
		const ImGuiIO& io = ImGui::GetIO();
		bool shiftPressed = io.KeyShift;
		bool ctrlPressed = io.KeyCtrl;

		if (shiftPressed)
		{
			// Transform last selection index into filtered space.
			int lastSelectedFilteredIdx = -1;
			for (int i = 0; i < filtered.size(); ++i)
			{
				if (filtered[i] == mLastSelectedIdx)
					lastSelectedFilteredIdx = i;
			}
			if (lastSelectedFilteredIdx == -1)
			{
				// If our last selection is no longer in filtered space, we can just treat it as a control key press.
				shiftPressed = false;
				ctrlPressed = true;
			}
			else
			{
				// Iteration must occur in filtered space to prevent selection of occluded nodes.
				const int a = std::min(justSelectedFilteredIdx, lastSelectedFilteredIdx);
				const int b = std::max(justSelectedFilteredIdx, lastSelectedFilteredIdx);

				for (int i = a; i <= b; ++i)
					if (!selMgr.isSelected(ESelection, sampler.rawAt(filtered[i])))
						selMgr.mSelections.emplace_back(ESelection, sampler.rawAt(filtered[i]));
			}
		}

		// If the control key was pressed, we add to the selection if necessary.
		if (ctrlPressed && !shiftPressed)
		{
			// If already selected, no action necessary - just for id update.
			if (!justSelectedAlreadySelected)
				selMgr.mSelections.emplace_back(ESelection, sampler.rawAt(justSelectedId));
			else if (thereWasAClick)
				selMgr.mSelections.erase(selMgr.mSelections.begin() + selMgr.getIdxOfSelect(ESelection, sampler.rawAt(justSelectedId)));
		}
		else if (!ctrlPressed && !shiftPressed && (thereWasAClick || justSelectedId != mLastSelectedIdx))
		{
			// Replace selection
			selMgr.mSelections.clear();
			selMgr.mSelections.emplace_back(ESelection, sampler.rawAt(justSelectedId));
		}

		// Set our last selection index, for shift clicks.
		mLastSelectedIdx = justSelectedId;
	}

private:
	int mLastSelectedIdx;
};
