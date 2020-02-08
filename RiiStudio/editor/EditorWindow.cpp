#include "EditorWindow.hpp"
#include <LibCore/api/Collection.hpp>

#include <LibCore/api/impl/api.hpp>

#include <regex>
#include <ThirdParty/fa5/IconsFontAwesome5.h>
#include <RiiStudio/editor/kit/Image.hpp>
#include <LibCore/windows/SelectionManager.hpp>

#include "Renderer.hpp"
#include <RiiStudio/editor/kit/Viewport.hpp>

struct GenericCollectionOutliner : public IStudioWindow
{
	GenericCollectionOutliner(px::CollectionHost& host) : IStudioWindow("Outliner"), mHost(host)
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
			char buf[128]{};
			memcpy_s(buf, 128, mFilt.c_str(), mFilt.length());
			ImGui::InputText(ICON_FA_SEARCH " (regex)", buf, 128);
			mFilt = buf;
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
	int calcNumFiltered(const px::CollectionHost::CollectionHandle& sampler, const TFilter* filter = nullptr) const noexcept
	{
		// If no data, empty
		if (sampler.size() == 0)
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
	std::string formatTitle(const px::CollectionHost::CollectionHandle& sampler, const TFilter* filter = nullptr) const noexcept
	{
		const auto name = GetRich(sampler.getType());
		return std::string(std::string(name.icon.icon_plural) + "  " +
			std::string(name.exposedName) + "s (" + std::to_string(calcNumFiltered(sampler, filter)) + ")");
	}

	void drawFolder(px::CollectionHost::CollectionHandle& sampler, px::CollectionHost& host, const std::string& key) noexcept
	{
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (!ImGui::TreeNode(formatTitle(sampler, &mFilter).c_str()))
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

			auto subHnds = px::ReflectionMesh::getInstance()->findParentOfType<px::CollectionHost>(sampler.atDynamic(i));


			if (!mFilter.test(name) && subHnds.empty())
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

			const std::string cur_name = std::string(GetRich(sampler.getType()).icon.icon_singular) + " " + std::string(name);
			
			if (ImGui::TreeNodeEx(std::to_string(i).c_str(), ImGuiTreeNodeFlags_DefaultOpen | (subHnds.empty() ? ImGuiTreeNodeFlags_Leaf : 0), cur_name.c_str()))
			{
				// NodeDrawer::drawNode(*node.get());

				for (int k = 0; k < subHnds.size(); ++k)
					drawRecursive(*subHnds[k]);

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
				const int a = std::min(justSelectedFilteredIdx, lastSelectedFilteredIdx);
				const int b = std::max(justSelectedFilteredIdx, lastSelectedFilteredIdx);

				for (int i = a; i <= b; ++i)
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

	void drawRecursive(px::CollectionHost& host) noexcept
	{
		for (u32 i = 0; i < host.getNumFolders(); ++i)
		{
			auto hnd = host.getFolder(i);
			drawFolder(hnd, host, hnd.getType());
		}
	}
	void draw() noexcept override
	{

		const auto& io = ImGui::GetIO();
		ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);
		mFilter.Draw();
		drawRecursive(mHost);
	}

private:
	px::CollectionHost& mHost;
	TFilter mFilter;
};

struct WIdek : public IStudioWindow
{
	WIdek() : IStudioWindow("Idek", false)
	{}
};

struct TexImgPreview : public IStudioWindow
{
	TexImgPreview(const px::CollectionHost& host)
		: IStudioWindow("Texture Preview"), mHost(host)
	{
		setWindowFlag(ImGuiWindowFlags_AlwaysAutoResize);
	}
	void draw() noexcept override
	{
		auto osamp = mHost.getFolder<lib3d::Texture>();

		assert(osamp.has_value());
		auto samp = osamp.value();

		if (lastTexId != samp.getActiveSelection())
		{
			const auto* at = samp.at<lib3d::Texture>(samp.getActiveSelection());
			if (at)
				setFromImage(*at);
		}
		mImg.draw();
	}
	void setFromImage(const lib3d::Texture& tex)
	{
		mImg.setFromImage(tex);
	}

	const px::CollectionHost& mHost;
	int lastTexId = -1;
	ImagePreview mImg;
};
struct RenderTest : public IStudioWindow
{
	RenderTest(const px::CollectionHost& host)
		: IStudioWindow("Render test"), mHost(host)
	{
		setWindowFlag(ImGuiWindowFlags_AlwaysAutoResize);

		mRenderer.prepare(*host.getFolder<px::CollectionHost>()->at<px::CollectionHost>(0));
	}
	void draw() noexcept override
	{

		auto bounds = ImGui::GetWindowSize();

		if (mViewport.begin(bounds.x, bounds.y))
		{
			mRenderer.render(bounds.x, bounds.y);
			mViewport.end();
		}
	}

	Viewport mViewport;
	Renderer mRenderer;
	const px::CollectionHost& mHost;
};
EditorWindow::EditorWindow(px::Dynamic state, const std::string& path)
	: mState({ std::move(state.mOwner), state.mBase, state.mType }), IStudioWindow(path.substr(path.rfind("\\")+1), true), mFilePath(path)
{
#if 0
	// TODO: Recursive..
	const auto hnd = px::ReflectionMesh::getInstance()->lookupInfo(
		mState.mType);
	assert(hnd);

	for (int i = 0; i < hnd.getNumParents(); ++i)
	{
		if (hnd.getParent(i).getName() == px::CollectionHost::TypeInfo.namespacedId)
		{
			px::CollectionHost* pHost = hnd.castToImmediateParent<px::CollectionHost>(mState.mBase, i);
			assert(pHost);
			if (!pHost) return;

		}
	}
#endif
	std::vector<px::CollectionHost*> collectionhosts= px::ReflectionMesh::getInstance()->findParentOfType<px::CollectionHost>(mState);

	if (collectionhosts.size() > 1)
	{
		printf("Cannot spawn editor window; too many collection hosts.\n");
	}
	else if (collectionhosts.empty())
	{
		printf("State has no collection hosts.\n");
	}
	else
	{
		assert(collectionhosts[0]);

		px::CollectionHost& host = *collectionhosts[0];


		attachWindow(std::make_unique<GenericCollectionOutliner>(host));
		attachWindow(std::make_unique<TexImgPreview>(host));
		attachWindow(std::make_unique<RenderTest>(host));

		
	}
}
void EditorWindow::draw() noexcept
{

}
