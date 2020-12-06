#include "Outliner.hpp"
#include <core/kpi/RichNameManager.hpp>     // kpi::RichNameManager
#include <frontend/editor/EditorWindow.hpp> // EditorWindow
#include <frontend/editor/StudioWindow.hpp> // StudioWindow
#include <plugins/gc/Export/Material.hpp>   // libcube::IGCMaterial
//#include <regex>                          // std::regex_search
#include <vendor/fa5/IconsFontAwesome5.h> // (const char*)ICON_FA_SEARCH

#include <core/kpi/ActionMenu.hpp> // kpi::ActionMenuManager

namespace riistudio::frontend {

struct GenericCollectionOutliner : public StudioWindow {
  GenericCollectionOutliner(kpi::INode& host, kpi::IObject*& active,
                            EditorWindow& ed);

  struct ImTFilter : public ImGuiTextFilter {
    bool test(const std::string& str) const noexcept {
      return PassFilter(str.c_str());
    }
  };
#if 0
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
      ImGui::InputText((const char*)ICON_FA_SEARCH " (regex)", buf, 128);
      mFilt = buf;
#endif
    }

    std::string mFilt;
  };
#endif
  using TFilter = ImTFilter;

  //! @brief Return the number of resources in the source that pass the filter.
  //!
  std::size_t calcNumFiltered(const kpi::ICollection& sampler,
                              const TFilter* filter = nullptr) const noexcept;

  //! @brief Format the title in the "<header> (<number of resources>)" format.
  //!
  std::string formatTitle(const kpi::ICollection& sampler,
                          const TFilter* filter = nullptr) const;

  void drawFolder(kpi::ICollection& sampler, const kpi::INode& host,
                  const std::string& key) noexcept;

  void drawRecursive(kpi::INode& host) noexcept;

private:
  void draw_() noexcept override;

  kpi::INode& mHost;
  TFilter mFilter;
  kpi::IObject*& mActive;
  EditorWindow& ed;

  kpi::IObject* activeModal = nullptr;
};

GenericCollectionOutliner::GenericCollectionOutliner(kpi::INode& host,
                                                     kpi::IObject*& active,
                                                     EditorWindow& ed)
    : StudioWindow("Outliner"), mHost(host), mActive(active), ed(ed) {
  setClosable(false);
}

std::size_t GenericCollectionOutliner::calcNumFiltered(
    const kpi::ICollection& sampler, const TFilter* filter) const noexcept {
  // If no data, empty
  if (sampler.size() == 0)
    return 0;

  // If we don't have a filter, everything is included.
  if (!filter)
    return sampler.size();

  std::size_t nPass = 0;

  for (u32 i = 0; i < sampler.size(); ++i)
    if (filter->test(sampler.atObject(i)->getName().c_str()))
      ++nPass;

  return nPass;
}

std::string
GenericCollectionOutliner::formatTitle(const kpi::ICollection& sampler,
                                       const TFilter* filter) const {
  if (sampler.size() == 0)
    return "";
  const auto rich =
      kpi::RichNameManager::getInstance().getRich(sampler.atObject(0));
  const std::string icon_plural = rich.getIconPlural();
  const std::string exposed_name = rich.getNamePlural();
  return std::string(icon_plural + "  " + exposed_name + " (" +
                     std::to_string(calcNumFiltered(sampler, filter)) + ")");
}

void GenericCollectionOutliner::drawFolder(kpi::ICollection& sampler,
                                           const kpi::INode& host,
                                           const std::string& key) noexcept {
  if (sampler.size() == 0)
    return;
  const auto rich =
      kpi::RichNameManager::getInstance().getRich(sampler.atObject(0));
  if (!rich.hasEntry())
    return;
  ImGui::SetNextItemOpen(true, ImGuiCond_Once);
  const bool opened = ImGui::TreeNode(formatTitle(sampler, &mFilter).c_str());
  if (!key.ends_with("Model") && !key.ends_with("Bone")) {
    const auto local_id = reinterpret_cast<u64>(&sampler);
    const auto id_str = std::string("MCtx") + std::to_string(local_id);
    if (ImGui::BeginPopupContextItem(id_str.c_str())) {
      ImGui::TextUnformatted(
          (rich.getIconPlural() + " " + rich.getNamePlural() + ": ").c_str());
      ImGui::Separator();

      {
        // set activeModal for an actual gui
        if (ImGui::MenuItem("Add New")) {
          sampler.add();
          ed.getDocument().commit();
        }
      }

      ImGui::EndPopup();
    }
  }
  if (!opened)
    return;

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
    auto& nodeAt = *sampler.atObject(i);
    std::string cur_name = nodeAt.getName();

    auto* as_host = dynamic_cast<kpi::INode*>(&nodeAt);

    if (as_host == nullptr && !mFilter.test(cur_name))
      continue;

    const auto rich = kpi::RichNameManager::getInstance().getRich(&nodeAt);

    if (!rich.hasEntry())
      continue;

    if (cur_name == "TODO") {
      cur_name = rich.getNameSingular() + " #" + std::to_string(i);
    }

    filtered.push_back(i);

    // Whether or not this node is already selected.
    // Selections from other windows will carry over.
    bool curNodeSelected = sampler.isSelected(i);

    const int icon_size = 24;

    ImGui::Selectable(std::to_string(i).c_str(), curNodeSelected,
                      ImGuiSelectableFlags_None, {0, icon_size});
    if (ImGui::BeginPopupContextItem(("Ctx" + std::to_string(i)).c_str())) {
      activeModal = &nodeAt;
      ImGui::TextUnformatted((rich.getIconSingular() + " " +
                              rich.getNameSingular() + ": " + cur_name)
                                 .c_str());
      ImGui::Separator();

      {
        if (kpi::ActionMenuManager::get().drawContextMenus(nodeAt))
          ed.getDocument().commit();

        // ImGui::PopID();
      }

      ImGui::EndPopup();
    }

    ImGui::SameLine();

    thereWasAClick = ImGui::IsItemClicked();
    bool focused = ImGui::IsItemFocused();

    lib3d::Texture* tex = dynamic_cast<lib3d::Texture*>(&nodeAt);
    libcube::IGCMaterial* mat = dynamic_cast<libcube::IGCMaterial*>(&nodeAt);

    u32 flags = ImGuiTreeNodeFlags_DefaultOpen;
    if (as_host == nullptr)
      flags |= ImGuiTreeNodeFlags_Leaf;

    const auto text_size = ImGui::CalcTextSize("T");
    const auto initial_pos_y = ImGui::GetCursorPosY();
    ImGui::SetCursorPosY(initial_pos_y + (icon_size - text_size.y) / 2);

    const auto treenode =
        ImGui::TreeNodeEx(std::to_string(i).c_str(), flags, "%s %s",
                          rich.getIconSingular().c_str(), cur_name.c_str());

    if (treenode) {

      if (mat != nullptr) {
        for (int s = 0; s < mat->getMaterialData().samplers.size(); ++s) {
          auto& sampl = *mat->getMaterialData().samplers[s].get();
          const lib3d::Texture* curImg = mat->getTexture(sampl.mTexture);
          ImGui::SameLine();
          ImGui::SetCursorPosY(initial_pos_y);
          ed.drawImageIcon(curImg, icon_size);
        }
      }
      if (tex != nullptr) {
        ImGui::SameLine();
        ImGui::SetCursorPosY(initial_pos_y);
        ed.drawImageIcon(tex, icon_size);
      }
    }
    ImGui::SetCursorPosY(initial_pos_y + icon_size);
    if (treenode) {
      // NodeDrawer::drawNode(*node.get());

      if (as_host != nullptr)
        drawRecursive(*as_host);

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

  // Unique selection model: No selections of different types.
  // Since we use type-folders, this means only one folder can have selections.
  // We only need to clear the folder of the last active object.
  if (mActive != nullptr && mActive->collectionOf != nullptr &&
      mActive->collectionOf != &sampler) {
    // Invalidate last selection, otherwise SHIFT anchors from the old folder
    // would carry over.
    sampler.setActiveSelection(~0);
    mActive->collectionOf->clearSelection();
  }

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
  mActive = sampler.atObject(justSelectedId); // TODO -- removing the active
                                              // node will cause issues here
}

void GenericCollectionOutliner::drawRecursive(kpi::INode& host) noexcept {
  for (int i = 0; i < host.numFolders(); ++i)
    drawFolder(*host.folderAt(i), host, host.idAt(i));
}

void GenericCollectionOutliner::draw_() noexcept {
  // activeModal = nullptr;
  mFilter.Draw();
  drawRecursive((kpi::INode&)mHost);

  if (activeModal != nullptr)
    if (kpi::ActionMenuManager::get().drawModals(*activeModal, &ed))
      ed.getDocument().commit();
}

std::unique_ptr<StudioWindow>
MakeOutliner(kpi::INode& host, kpi::IObject*& active, EditorWindow& ed) {
  return std::make_unique<GenericCollectionOutliner>(host, active, ed);
}

} // namespace riistudio::frontend
