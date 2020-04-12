#include "editor_window.hpp"

#include <random>
#include <regex>

#include "Renderer.hpp"
#include "kit/Image.hpp"
#include "kit/Viewport.hpp"
#include <vendor/fa5/IconsFontAwesome5.h>

#include <plugins/gc/Export/Material.hpp>

#ifdef _WIN32
#include <vendor/glfw/glfw3.h>
#else
#include <SDL.h>

#include <emscripten.h>

#include <emscripten/html5.h>
#endif

#include <core/kpi/PropertyView.hpp>
#include <core/kpi/RichNameManager.hpp>

#include <algorithm>

#undef near

// Web security...
#ifdef __EMSCRIPTEN__
extern bool gPointerLock;
#endif

namespace riistudio::frontend {

inline bool ends_with(const std::string& value, const std::string& ending) {
  return ending.size() <= value.size() &&
         std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

struct GenericCollectionOutliner : public StudioWindow {
  GenericCollectionOutliner(kpi::IDocumentNode& host,
                            kpi::IDocumentNode*& active)
      : StudioWindow("Outliner"), mHost(host), mActive(active) {}

  struct ImTFilter : public ImGuiTextFilter {
    bool test(const std::string& str) const noexcept {
      return PassFilter(str.c_str());
    }
  };
  struct RegexFilter {
    bool test(const std::string& str) const noexcept {
      try {
        std::regex match(mFilt);
        return std::regex_search(str, match);
      } catch (std::exception e) {
        return false;
      }
    }
    void Draw() {
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

  //! @brief Return the number of resources in the source that pass the filter.
  //!
  std::size_t calcNumFiltered(const kpi::FolderData& sampler,
                              const TFilter* filter = nullptr) const noexcept {
    // If no data, empty
    if (sampler.size() == 0)
      return 0;

    // If we don't have a filter, everything is included.
    if (!filter)
      return sampler.size();

    std::size_t nPass = 0;

    for (u32 i = 0; i < sampler.size(); ++i)
      if (filter->test(sampler[i]->getName().c_str()))
        ++nPass;

    return nPass;
  }

  //! @brief Format the title in the "<header> (<number of resources>)" format.
  //!
  std::string formatTitle(const kpi::FolderData& sampler,
                          const TFilter* filter = nullptr) const {
    const auto rich = kpi::RichNameManager::getInstance().getRich(
        &sampler.at<kpi::IDocumentNode>(0));
    const std::string icon_plural = rich.getIconPlural();
    const std::string exposed_name = rich.getNamePlural();
    return std::string(icon_plural + "  " + exposed_name + " (" +
                       std::to_string(calcNumFiltered(sampler, filter)) + ")");
  }

  void drawFolder(kpi::FolderData& sampler, const kpi::IDocumentNode& host,
                  const std::string& key) noexcept {
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (!ImGui::TreeNode(formatTitle(sampler, &mFilter).c_str()))
      return;

    //	if (ImGui::Button("Shuffle")) {
    //		static std::default_random_engine rng{};
    //		std::shuffle(std::begin(sampler), std::end(sampler), rng);
    //	}

    // A filter tree for multi selection. Prevents inclusion of unfiltered data
    // with SHIFT clicks.
    std::vector<int> filtered;

    int justSelectedId = -1;
    // Relative to filter vector.
    std::size_t justSelectedFilteredIdx = -1;
    // Necessary to filter out clicks on already selected items.
    bool justSelectedAlreadySelected = false;
    // Prevent resetting when SHIFT is unpressed with arrow keys.
    bool thereWasAClick = false;

    // Draw the tree
    for (int i = 0; i < sampler.size(); ++i) {
      auto& nodeAt = sampler[i];
      const std::string cur_name = nodeAt->getName();

      if (nodeAt->children.empty() && !mFilter.test(cur_name)) {
        continue;
      }

      filtered.push_back(i);

      // Whether or not this node is already selected.
      // Selections from other windows will carry over.
      bool curNodeSelected = sampler.isSelected(i);

      bool bSelected =
          ImGui::Selectable(std::to_string(i).c_str(), curNodeSelected);
      ImGui::SameLine();

      thereWasAClick = ImGui::IsItemClicked();
      bool focused = ImGui::IsItemFocused();

      const auto rich =
          kpi::RichNameManager::getInstance().getRich(nodeAt.get());

      if (ImGui::TreeNodeEx(
              std::to_string(i).c_str(),
              ImGuiTreeNodeFlags_DefaultOpen |
                  (nodeAt->children.empty() ? ImGuiTreeNodeFlags_Leaf : 0),
              (rich.getIconSingular() + " " + cur_name).c_str())) {
        // NodeDrawer::drawNode(*node.get());

        drawRecursive(*nodeAt.get());

        // If there waas a click above, we need to ignore the focus below.
        // Assume only one item can be clicked.
        if (thereWasAClick ||
            (focused && justSelectedId == -1 && !curNodeSelected)) {
          // If it was already selected, we may need to reprocess for ctrl
          // clicks followed by shift clicks
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

    if (shiftPressed) {
      // Transform last selection index into filtered space.
      std::size_t lastSelectedFilteredIdx = -1;
      for (std::size_t i = 0; i < filtered.size(); ++i) {
        if (filtered[i] == sampler.getActiveSelection())
          lastSelectedFilteredIdx = i;
      }
      if (lastSelectedFilteredIdx == -1) {
        // If our last selection is no longer in filtered space, we can just
        // treat it as a control key press.
        shiftPressed = false;
        ctrlPressed = true;
      } else {
#undef min
#undef max
        // Iteration must occur in filtered space to prevent selection of
        // occluded nodes.
        const std::size_t a =
            std::min(justSelectedFilteredIdx, lastSelectedFilteredIdx);
        const std::size_t b =
            std::max(justSelectedFilteredIdx, lastSelectedFilteredIdx);

        for (std::size_t i = a; i <= b; ++i)
          sampler.select(filtered[i]);
      }
    }

    // If the control key was pressed, we add to the selection if necessary.
    if (ctrlPressed && !shiftPressed) {
      // If already selected, no action necessary - just for id update.
      if (!justSelectedAlreadySelected)
        sampler.select(justSelectedId);
      else if (thereWasAClick)
        sampler.deselect(justSelectedId);
    } else if (!ctrlPressed && !shiftPressed &&
               (thereWasAClick ||
                justSelectedId != sampler.getActiveSelection())) {
      // Replace selection
      sampler.clearSelection();
      sampler.select(justSelectedId);
    }

    // Set our last selection index, for shift clicks.
    sampler.setActiveSelection(justSelectedId);
    mActive =
        sampler[justSelectedId]
            .get(); // TODO -- removing the active node will cause issues here
  }

  void drawRecursive(kpi::IDocumentNode& host) noexcept {
    for (auto& folder : host.children)
      drawFolder(folder.second, host, folder.first);
  }
  void draw_() noexcept override {
    mFilter.Draw();
    drawRecursive(mHost);
  }

private:
  kpi::IDocumentNode& mHost;
  TFilter mFilter;
  kpi::IDocumentNode*& mActive;
};

struct RenderTest : public StudioWindow {

  RenderTest(const kpi::IDocumentNode& host)
      : StudioWindow("Viewport"), mHost(host) {
    setWindowFlag(ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_MenuBar);

    const auto* models = host.getFolder<lib3d::Model>();

    if (!models)
      return;

    if (models->size() == 0)
      return;

    model = &models->at<kpi::IDocumentNode>(0);
    mRenderer.prepare(*model, host);
  }
  void draw_() override {

    auto bounds = ImGui::GetWindowSize();

    if (mViewport.begin(static_cast<u32>(bounds.x),
                        static_cast<u32>(bounds.y))) {
      auto* parent = dynamic_cast<EditorWindow*>(mParent);
      static bool showCursor = false; // TODO
      mRenderer.render(static_cast<u32>(bounds.x), static_cast<u32>(bounds.y),
                       showCursor);

#ifdef RII_BACKEND_GLFW
      if (mpGlfwWindow != nullptr)
        glfwSetInputMode(mpGlfwWindow, GLFW_CURSOR,
                         showCursor ? GLFW_CURSOR_NORMAL
                                    : GLFW_CURSOR_DISABLED);
#elif defined(RII_BACKEND_SDL) && defined(__EMSCRIPTEN__)
      gPointerLock = !showCursor;
      if (showCursor)
        emscripten_exit_pointerlock();
      else
        emscripten_request_pointerlock(0, EM_TRUE);
        // SDL_SetRelativeMouseMode(!showCursor ? SDL_TRUE : SDL_FALSE);
#endif
      mViewport.end();
    }
  }
  Viewport mViewport;
  Renderer mRenderer;
  const kpi::IDocumentNode* model;
  const kpi::IDocumentNode& mHost; // texture
};

struct HistoryList : public StudioWindow {
  HistoryList(kpi::History& host, kpi::IDocumentNode& root)
      : StudioWindow("History"), mHost(host), mRoot(root) {}
  void draw_() override {
    if (ImGui::Button("Commit " ICON_FA_SAVE)) {
      mHost.commit(mRoot);
    }
    ImGui::SameLine();
    if (ImGui::Button("Undo " ICON_FA_UNDO)) {
      mHost.undo(mRoot);
    }
    ImGui::SameLine();
    if (ImGui::Button("Redo " ICON_FA_REDO)) {
      mHost.redo(mRoot);
    }
    ImGui::BeginChild("Record List");
    for (std::size_t i = 0; i < mHost.size(); ++i) {
      ImGui::Text("(%s) History #%u", i == mHost.cursor() ? "X" : " ",
                  static_cast<u32>(i));
    }
    ImGui::EndChild();
  }

  kpi::History& mHost;
  kpi::IDocumentNode& mRoot;
};

struct PropertyEditor : public StudioWindow {
  PropertyEditor(kpi::History& host, kpi::IDocumentNode& root,
                 kpi::IDocumentNode*& active)
      : StudioWindow("Property Editor"), mHost(host), mRoot(root),
        mActive(active) {
    setWindowFlag(ImGuiWindowFlags_MenuBar);
  }

  template <typename T>
  void gatherSelected(std::vector<kpi::IDocumentNode*>& tmp,
                      kpi::FolderData& folder, T& pred) {
    for (int i = 0; i < folder.size(); ++i) {
      if (folder.isSelected(i) && pred(folder[i].get())) {
        tmp.push_back(folder[i].get());
      }

      for (auto& subfolder : folder[i]->children) {
        gatherSelected(tmp, subfolder.second, pred);
      }
    }
  }
  void draw_() override {
    auto& manager = kpi::PropertyViewManager::getInstance();

    if (mActive == nullptr) {
      ImGui::Text("Nothing is selected.");
      return;
    }
    std::vector<kpi::IDocumentNode*> selected;
    for (auto& subfolder : mRoot.children) {
      gatherSelected(selected, subfolder.second, [&](kpi::IDocumentNode* node) {
        return node->mType == mActive->mType;
      });
    }

    kpi::IPropertyView* activeTab = nullptr;

    if (selected.empty()) {
      ImGui::Text(ICON_FA_EXCLAMATION_TRIANGLE
                  " Active selection and multiselection desynced. This "
                  "shouldn't happen.");
      selected.push_back(mActive);
    }
    ImGui::Text("%s %s (%u)", mActive->getName().c_str(),
                selected.size() > 1 ? "..." : "",
                static_cast<u32>(selected.size()));

    if (ImGui::BeginTabBar("Pane")) {
      int i = 0;
      std::string title;
      manager.forEachView(
          [&](kpi::IPropertyView& view) {
            const bool sel = mActiveTab == i;

            title.clear();
            title += view.getIcon();
            title += " ";
            title += view.getName();

            if (ImGui::BeginTabItem(title.c_str())) {
              mActiveTab = i;
              activeTab = &view;
              ImGui::EndTabItem();
            }

            ++i;
          },
          *mActive);
      ImGui::EndTabBar();
    }

    if (activeTab == nullptr) {
      mActiveTab = 0;

      ImGui::Text("Invalid Pane");
      return;
    }

    activeTab->draw(*mActive, selected, mHost, mRoot);
  }

  kpi::History& mHost;
  kpi::IDocumentNode& mRoot;

  kpi::IDocumentNode*& mActive;

  int mActiveTab = 0;
};

EditorWindow::EditorWindow(std::unique_ptr<kpi::IDocumentNode> state,
                           const std::string& path)
    : StudioWindow(path.substr(path.rfind("\\") + 1), true),
      mState(std::move(state)), mFilePath(path) {
  mHistory.commit(*mState.get());

  attachWindow(
      std::make_unique<PropertyEditor>(mHistory, *mState.get(), mActive));
  attachWindow(std::make_unique<HistoryList>(mHistory, *mState.get()));
  attachWindow(
      std::make_unique<GenericCollectionOutliner>(*mState.get(), mActive));
  attachWindow(std::make_unique<RenderTest>(*mState.get()));
}
void EditorWindow::draw_() {
#ifdef _WIN32
  if (ImGui::GetIO().KeyCtrl) {
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z))) {
      mHistory.undo(*mState.get());
    } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Y))) {
      mHistory.redo(*mState.get());
    }
  }
#endif
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
} // namespace riistudio::frontend
