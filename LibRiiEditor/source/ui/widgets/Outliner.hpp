#pragma once

#include <string>
#include <memory>
#include <vector>
#include <imgui/imgui.h>
#include "core/SelectionManager.hpp"

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

struct OutlinerConfigDefault
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

template<typename T>
struct OutlinerNodeDrawerDefault
{
	inline void drawNode(T&) {}
};
//! @brief	Successor to TTreeMultiSelect.
//!			This version has minimal retained state: selection managers and data sources are not attached.
//!			Represents a folder in an outliner of one type.
//!
//! @tparam TResource	Type of resource to accept.
//! @tparam ESelection	Selection type for manager.
//! @tparam TConfig		Configuration, used to query icons and names.
//! @tparam NodeDrawer	Draws additional data for nodes in tree.
//!
template<class TResource, SelectionManager::Type ESelection, typename TConfig = OutlinerConfigDefault, typename NodeDrawer=OutlinerNodeDrawerDefault<TResource>>
class OutlinerFolder
{
public:
	using res_t = std::unique_ptr<TResource>;
	using res_vector_t = std::vector<res_t>;

	OutlinerFolder() = default;
	~OutlinerFolder() = default;

	int mLastSelectedIdx;


	//! @brief Return the number of resources in the source that pass the filter.
	//!
	int calcNumFiltered(const res_vector_t& vec, const ImGuiTextFilter* filter = nullptr) const noexcept
	{
		// If no data, empty
		if (vec.empty())
			return 0;

		// If we don't have a filter, everything is included.
		if (!filter)
			return vec.size();

		u32 nPass = 0;

		for (const auto& res : vec)
			if (filter->PassFilter(res->getName().c_str()))
				nPass++;

		return nPass;
	}

	//! @brief Format the title in the "<header> (<number of resources>)" format.
	//!
	std::string formatTitle(const res_vector_t& vec, const ImGuiTextFilter* filter = nullptr) const noexcept
	{
		int numFiltered = calcNumFiltered(vec, filter);
		return TConfig::res_icon_plural() + "  " + TConfig::res_name_plural() + " (" + std::to_string(numFiltered) + ")";
	}

	// TODO: Move to it's own utility header.
	struct ScopedIncrementer
	{
		ScopedIncrementer(int& i)
			: mI(i)
		{}
		~ScopedIncrementer()
		{
			++mI;
		}

		int& mI;
	};

	void draw(const res_vector_t& vec, SelectionManager& selMgr, const ImGuiTextFilter* pFilter = nullptr)
	{
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (!ImGui::TreeNode(formatTitle(vec, pFilter).c_str()))
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
		int i = 0;
		for (const auto& node : vec)
		{
			ScopedIncrementer scoped(i);

			if (pFilter && !pFilter->PassFilter(node->getName().c_str()))
				continue;

			filtered.push_back(i);

			// Whether or not this node is already selected.
			// Selections from other windows will carry over.
			bool curNodeSelected = selMgr.isSelected(ESelection, node.get());

			bool bSelected = ImGui::Selectable(std::to_string(i).c_str(), curNodeSelected);
			ImGui::SameLine();

			thereWasAClick = ImGui::IsItemClicked();
			bool focused = ImGui::IsItemFocused();

			const std::string cur_name = TConfig::res_icon_singular() + " " + node->getName();

			if (ImGui::TreeNodeEx(std::to_string(i).c_str(), ImGuiTreeNodeFlags_Leaf, cur_name.c_str()))
			{
				NodeDrawer::drawNode(*node.get());

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
					if (!selMgr.isSelected(ESelection, vec[filtered[i]].get()))
						selMgr.mSelections.emplace_back(ESelection, vec[filtered[i]].get());
			}
		}

		// If the control key was pressed, we add to the selection if necessary.
		if (ctrlPressed && !shiftPressed)
		{
			// If already selected, no action necessary - just for id update.
			if (!justSelectedAlreadySelected)
				selMgr.mSelections.emplace_back(ESelection, vec[justSelectedId].get());
			else if (thereWasAClick)
				selMgr.mSelections.erase(selMgr.mSelections.begin() + selMgr.getIdxOfSelect(ESelection, vec[justSelectedId].get()));
		}
		else if (!ctrlPressed && !shiftPressed && (thereWasAClick || justSelectedId != mLastSelectedIdx))
		{
			// Replace selection
			selMgr.mSelections.clear();
			selMgr.mSelections.emplace_back(ESelection, vec[justSelectedId].get());
		}

		// Set our last selection index, for shift clicks.
		mLastSelectedIdx = justSelectedId;
	}
};
