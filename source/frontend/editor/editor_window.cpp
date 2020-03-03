#include "editor_window.hpp"

#include <regex>
#include <vendor/fa5/IconsFontAwesome5.h>
#include "kit/Image.hpp"
#include "Renderer.hpp"
#include "kit/Viewport.hpp"

namespace riistudio::frontend {

struct GenericCollectionOutliner : public StudioWindow
{
	GenericCollectionOutliner(kpi::IDocumentNode& host) : StudioWindow("Outliner"), mHost(host)
	{}

	struct ImTFilter : public ImGuiTextFilter
	{
		bool test(const std::string& str) const noexcept
		{
			return PassFilter(str.c_str());
		}
	};
	struct RegexFilter
	{
		bool test(const std::string& str) const noexcept
		{
			try {
				std::regex match(mFilt);
				return std::regex_search(str, match);
			}
			catch (std::exception e)
			{
				return false;
			}
		}
		void Draw()
		{
#ifdef _WIN32
			char buf[128]{};
			memcpy_s(buf, 128, mFilt.c_str(), mFilt.length());
			ImGui::InputText(ICON_FA_SEARCH " (regex)", buf, 128);
			mFilt = buf;
#endif
		}

		std::string mFilt;
	};
	using TFilter = RegexFilter;

	struct TConfig
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

	//! @brief Return the number of resources in the source that pass the filter.
	//!
	std::size_t calcNumFiltered(const kpi::FolderData& sampler, const TFilter* filter = nullptr) const noexcept
	{
		// If no data, empty
		if (sampler.size() == 0)
			return 0;

		// If we don't have a filter, everything is included.
		if (!filter)
			return sampler.size();

		std::size_t nPass = 0;

		//for (u32 i = 0; i < sampler.size(); ++i)
		//	if (filter->test(sampler.nameAt(i).c_str()))
		//		++nPass;

		return nPass;
	}

	//! @brief Format the title in the "<header> (<number of resources>)" format.
	//!
	std::string formatTitle(const kpi::FolderData& sampler, const TFilter* filter = nullptr) const noexcept
	{
		return "";
		//	const auto name = GetRich(sampler.getType());
		//	return std::string(std::string(name.icon.icon_plural) + "  " +
		//		std::string(name.exposedName) + "s (" + std::to_string(calcNumFiltered(sampler, filter)) + ")");
	}

	void drawFolder(kpi::FolderData& sampler, const kpi::IDocumentNode& host, const std::string& key) noexcept
	{
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (!ImGui::TreeNode(formatTitle(sampler, &mFilter).c_str()))
			return;


		// A filter tree for multi selection. Prevents inclusion of unfiltered data with SHIFT clicks.
		std::vector<int> filtered;

		int justSelectedId = -1;
		// Relative to filter vector.
		std::size_t justSelectedFilteredIdx = -1;
		// Necessary to filter out clicks on already selected items.
		bool justSelectedAlreadySelected = false;
		// Prevent resetting when SHIFT is unpressed with arrow keys.
		bool thereWasAClick = false;

		// Draw the tree
		for (int i = 0; i < sampler.size(); ++i)
		{
			std::string name = "TODO"; // sampler.nameAt(i);

			if (!mFilter.test(name) /* TODO: Check if has children.. */)
			{
				continue;
			}

			filtered.push_back(i);

			// Whether or not this node is already selected.
			// Selections from other windows will carry over.
			bool curNodeSelected = sampler.isSelected(i);

			bool bSelected = ImGui::Selectable(std::to_string(i).c_str(), curNodeSelected);
			ImGui::SameLine();

			thereWasAClick = ImGui::IsItemClicked();
			bool focused = ImGui::IsItemFocused();

			std::string cur_name = sampler.type;
			// std::string cur_name = "TODO";
			//	std::string(GetRich(sampler.getType()).icon.icon_singular) + " " +
			//	name;

			if (ImGui::TreeNodeEx(std::to_string(i).c_str(), ImGuiTreeNodeFlags_DefaultOpen | (false /* TODO */ ? ImGuiTreeNodeFlags_Leaf : 0), cur_name.c_str()))
			{
				// NodeDrawer::drawNode(*node.get());

				drawRecursive(*sampler[i].get());

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
			std::size_t lastSelectedFilteredIdx = -1;
			for (std::size_t i = 0; i < filtered.size(); ++i)
			{
				if (filtered[i] == sampler.getActiveSelection())
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
#undef min
#undef max
				// Iteration must occur in filtered space to prevent selection of occluded nodes.
				const std::size_t a = std::min(justSelectedFilteredIdx, lastSelectedFilteredIdx);
				const std::size_t b = std::max(justSelectedFilteredIdx, lastSelectedFilteredIdx);

				for (std::size_t i = a; i <= b; ++i)
					sampler.select(filtered[i]);
			}
		}

		// If the control key was pressed, we add to the selection if necessary.
		if (ctrlPressed && !shiftPressed)
		{
			// If already selected, no action necessary - just for id update.
			if (!justSelectedAlreadySelected)
				sampler.select(justSelectedId);
			else if (thereWasAClick)
				sampler.deselect(justSelectedId);
		}
		else if (!ctrlPressed && !shiftPressed && (thereWasAClick || justSelectedId != sampler.getActiveSelection()))
		{
			// Replace selection
			sampler.clearSelection();
			sampler.select(justSelectedId);
		}

		// Set our last selection index, for shift clicks.
		sampler.setActiveSelection(justSelectedId);
	}

	void drawRecursive(kpi::IDocumentNode& host) noexcept
	{
		for (auto folder : host.children)
			drawFolder(folder.second, host, folder.first);
	}
	void draw_() noexcept override
	{

		mFilter.Draw();
		drawRecursive(mHost);
	}

private:
	kpi::IDocumentNode& mHost;
	TFilter mFilter;
};

struct TexImgPreview : public StudioWindow
{
	TexImgPreview(const kpi::IDocumentNode& host)
		: StudioWindow("Texture Preview"), mHost(host)
	{
		setWindowFlag(ImGuiWindowFlags_AlwaysAutoResize);
	}
	void draw_() override
	{
		auto* osamp = mHost.getFolder<lib3d::Texture>();
		if (!osamp) return;

		assert(osamp);
		auto& samp = *osamp;

		if (lastTexId != samp.getActiveSelection())
			setFromImage(samp.at<lib3d::Texture>(samp.getActiveSelection()));

		mImg.draw();
	}
	void setFromImage(const lib3d::Texture& tex)
	{
		mImg.setFromImage(tex);
	}

	const kpi::IDocumentNode& mHost;
	int lastTexId = -1;
	ImagePreview mImg;
};
struct RenderTest : public StudioWindow
{
	RenderTest(const kpi::IDocumentNode& host)
		: StudioWindow("Viewport"), mHost(host)
	{
		setWindowFlag(ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_MenuBar);

		const auto* models = host.getFolder<kpi::IDocumentNode>();

		if (!models)
			return;

		if (models->size() == 0)
			return;

		mRenderer.prepare(models->at<kpi::IDocumentNode>(0), host);
	}
	void draw_() override
	{

		auto bounds = ImGui::GetWindowSize();

		if (mViewport.begin(static_cast<u32>(bounds.x), static_cast<u32>(bounds.y)))
		{
			auto* parent = dynamic_cast<EditorWindow*>(mParent);
			bool showCursor = false;
			mRenderer.render(static_cast<u32>(bounds.x), static_cast<u32>(bounds.y), showCursor);
			mViewport.end();
		}
	}
	Viewport mViewport;
	Renderer mRenderer;
	const kpi::IDocumentNode& mHost;
};
EditorWindow::EditorWindow(std::unique_ptr<kpi::IDocumentNode> state, const std::string& path)
	: StudioWindow(path.substr(path.rfind("\\") + 1), true), mState(std::move(state)), mFilePath(path)
{
	mHistory.commit(*mState.get());

	attachWindow(std::make_unique<GenericCollectionOutliner>(*mState.get()));
	attachWindow(std::make_unique<TexImgPreview>(*mState.get()));
	attachWindow(std::make_unique<RenderTest>(*mState.get()));
}
void EditorWindow::draw_()
{
	auto* parent = mParent;

	// if (!parent) return;

	
	//	if (!showCursor)
	//	{
	//		parent->hideMouse();
	//	}
	//	else
	//	{
	//		parent->showMouse();
	//	}
}
}
