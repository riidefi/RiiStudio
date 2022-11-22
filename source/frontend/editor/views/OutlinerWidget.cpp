#include "OutlinerWidget.hpp"
#include <frontend/editor/EditorWindow.hpp>

namespace riistudio::frontend {

std::size_t CalcNumFiltered(const NodeFolder& folder, const TFilter* filter) {
  // If no data, empty
  if (folder.children.size() == 0)
    return 0;

  // If we don't have a filter, everything is included.
  if (filter == nullptr)
    return folder.children.size();

  std::size_t nPass = 0;

  for (auto& c : folder.children)
    if (filter->test(c->public_name))
      ++nPass;

  return nPass;
}

std::string FormatTitle(const NodeFolder& folder, const TFilter* filter) {
  if (folder.children.size() == 0)
    return "";

  const std::string& icon_plural = folder.type_icon_pl;
  const std::string& exposed_name = folder.type_name_pl;

  return std::string(icon_plural + "  " + exposed_name + " (" +
                     std::to_string(CalcNumFiltered(folder, filter)) + ")");
}

bool isSelected(const SelectionManager& ed_win, NodeFolder& nodes,
                size_t index) {
  return ed_win.isSelected(nodes.children[index]->obj);
}
void select(SelectionManager& ed_win, NodeFolder& nodes, size_t index) {
  ed_win.select(nodes.children[index]->obj);
}
void deselect(SelectionManager& ed_win, NodeFolder& nodes, size_t index) {
  ed_win.deselect(nodes.children[index]->obj);
}

size_t getActiveSelection(const SelectionManager& ed_win, NodeFolder& nodes) {
  auto* obj = ed_win.getActive();

  for (int i = 0; i < nodes.children.size(); ++i)
    if (nodes.children[i]->obj == obj)
      return i;

  return ~0;
}
void setActiveSelection(SelectionManager& ed_win, NodeFolder& nodes,
                        size_t index) {
  if (index == ~0)
    ed_win.setActive(nullptr);
  else
    ed_win.setActive(nodes.children[index]->obj);
}

void DrawNodePic(EditorWindow& ed, Child& child, float initial_pos_y,
                 int icon_size) {
  for (auto* pImg : child.icons_right) {
    ImGui::SameLine();
    ImGui::SetCursorPosY(initial_pos_y);
    ed.drawImageIcon(pImg, icon_size);
  }
}

// For models and bones we disable "add new" functionality for some reason
static bool CanCreateNew(std::string_view key) {
  return !key.ends_with("Model") && !key.ends_with("Bone");
}

void AddNewCtxMenu(EditorWindow& ed, Child::Folder& folder) {
  if (CanCreateNew(folder.key)) {
    const auto id_str = std::string("MCtx") + folder.key;
    if (ImGui::BeginPopupContextItem(id_str.c_str())) {
      ImGui::TextUnformatted(
          (folder.type_icon_pl + " " + folder.type_name_pl + ": ").c_str());
      ImGui::Separator();

      {
        // set activeModal for an actual gui
        if (ImGui::MenuItem("Add New")) {
          folder.add_new_fn();
          // TODO: Move commit inside?

          // Selection must be reset
          ed.getDocument().commit(ed.getSelection(), true);
        }
      }

      ImGui::EndPopup();
    }
  }
}

void DrawFolder(NodeFolder& folder, TFilter& mFilter, EditorWindow& ed,
                std::optional<std::function<void()>>& activeModal,
                kpi::IObject*& mActive, std::string& mActiveClassId) {
  if (folder.children.size() == 0)
    return;

  if (!folder.children[0].has_value() || !folder.children[0]->is_rich)
    return;

  bool opened = false;

  if (folder.key != "ROOT") {
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    opened = ImGui::TreeNode(FormatTitle(folder, &mFilter).c_str());
    AddNewCtxMenu(ed, folder);
    if (!opened)
      return;
  }

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

  auto& children = folder.children;

  auto& sel = ed.getSelection();

  // Draw the tree
  for (int i = 0; i < children.size(); ++i) {
    if (!children[i].has_value()) {
      continue;
    }

    // auto& nodeAt = *children[i]->obj;
    std::string cur_name = children[i]->public_name;

    if (!children[i]->is_container && !mFilter.test(children[i]->public_name))
      continue;

    // TODO: Do we still need this?
    if (!children[i]->is_rich)
      continue;

    filtered.push_back(i);

    // Whether or not this node is already selected.
    // Selections from other windows will carry over.
    bool curNodeSelected = isSelected(sel, folder, i);

    const float icon_size = 24.0f * ImGui::GetIO().FontGlobalScale;

    ImGui::Selectable(std::to_string(i).c_str(), curNodeSelected,
                      ImGuiSelectableFlags_None, {0, icon_size});
    if (ImGui::BeginPopupContextItem(("Ctx" + std::to_string(i)).c_str())) {

      activeModal = [draw_modal = children[i]->draw_modal_fn, ed = &ed]() {
        draw_modal(*ed);
      };
      ImGui::TextUnformatted((children[i]->type_icon + " " +
                              children[i]->type_name + ": " + cur_name)
                                 .c_str());
      ImGui::Separator();

      children[i]->draw_context_menu_fn(ed);

      // set activeModal for an actual gui
      if (folder.delete_child_fn != nullptr && ImGui::MenuItem("Delete")) {
        children[i]->mark_to_delete = true;
      }

      ImGui::EndPopup();
    }

    ImGui::SameLine();

    thereWasAClick = ImGui::IsItemClicked();
    bool focused = ImGui::IsItemFocused();

    u32 flags = ImGuiTreeNodeFlags_DefaultOpen;
    if (!children[i]->is_container)
      flags |= ImGuiTreeNodeFlags_Leaf;

    const auto text_size = ImGui::CalcTextSize("T");
    const auto initial_pos_y = ImGui::GetCursorPosY();
    ImGui::SetCursorPosY(initial_pos_y + (icon_size - text_size.y) / 2);

    const auto treenode =
        ImGui::TreeNodeEx(std::to_string(i).c_str(), flags, "%s %s",
                          children[i]->type_icon.c_str(), cur_name.c_str());
    if (treenode) {
      DrawNodePic(ed, *children[i], initial_pos_y, icon_size);
    }
    ImGui::SetCursorPosY(initial_pos_y + icon_size);
    if (treenode) {
      // NodeDrawer::drawNode(*node.get());

      // drawRecursive(children[i]->folders);
      for (auto& folder : children[i]->folders)
        DrawFolder(folder, mFilter, ed, activeModal, mActive, mActiveClassId);

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
  if (folder.key != "ROOT") {
    ImGui::TreePop();
  }

  for (int i = 0; i < children.size(); ++i) {
    if (!children[i]->mark_to_delete)
      continue;
    folder.delete_child_fn(i);
    folder.children.erase(folder.children.begin() + i);
    // Selection must be reset
    ed.getDocument().commit(ed.getSelection(), true);
    justSelectedId = -1;
    return;
  }

  // If nothing new was selected, no new processing needs to occur.
  if (justSelectedId == -1)
    return;

  // Unique selection model: No selections of different types.
  // Since we use type-folders, this means only one folder can have selections.
  // We only need to clear the folder of the last active object.
  if (mActive != nullptr && !mActiveClassId.empty() &&
      mActiveClassId != folder.key) {
    // Invalidate last selection, otherwise SHIFT anchors from the old folder
    // would carry over.
    setActiveSelection(sel, folder, ~0);

    // TODO: Wrap this up
    sel.selected.clear();
    // mActive->collectionOf->clearSelection();
  }
  mActiveClassId = folder.key;

  // Currently, nothing for ctrl + shift or ctrl + art.
  // If both are pressed, SHIFT takes priority.
  const ImGuiIO& io = ImGui::GetIO();
  bool shiftPressed = io.KeyShift;
  bool ctrlPressed = io.KeyCtrl;

  if (shiftPressed) {
    // Transform last selection index into filtered space.
    std::size_t lastSelectedFilteredIdx = -1;
    for (std::size_t i = 0; i < filtered.size(); ++i) {
      if (filtered[i] == getActiveSelection(sel, folder))
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
        select(sel, folder, filtered[i]);
    }
  }

  // If the control key was pressed, we add to the selection if necessary.
  if (ctrlPressed && !shiftPressed) {
    // If already selected, no action necessary - just for id update.
    if (!justSelectedAlreadySelected)
      select(sel, folder, justSelectedId);
    else if (thereWasAClick)
      deselect(sel, folder, justSelectedId);
  }

  // Set our last selection index, for shift clicks.
  setActiveSelection(sel, folder, justSelectedId);

  // This sets a member inside EditorWindow
  mActive = folder.children[justSelectedId]->obj; // TODO -- removing the active
                                                  // node will cause issues here

  if (!ctrlPressed && !shiftPressed) {
    sel.selected.clear();
  }

  sel.select(mActive);
}

} // namespace riistudio::frontend
