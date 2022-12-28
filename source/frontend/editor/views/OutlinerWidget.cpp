#include "OutlinerWidget.hpp"
#include <frontend/editor/EditorWindow.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

namespace riistudio::frontend {

// For models and bones we disable "add new" functionality for some reason
static bool CanCreateNew(std::string_view key) {
  return !key.ends_with("Model") && !key.ends_with("Bone");
}
bool ShouldBeDefaultOpen(const NodeFolder& folder) {
  // Polygons
  if (folder.type_icon_pl == (const char*)ICON_FA_DRAW_POLYGON)
    return false;
  // Vertex Colors
  if (folder.type_icon_pl == (const char*)ICON_FA_BRUSH)
    return false;

  return true;
}

std::size_t OutlinerWidget::CalcNumFiltered(const NodeFolder& folder,
                                            const TFilter* filter) {
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

std::string OutlinerWidget::FormatTitle(const NodeFolder& folder,
                                        const TFilter* filter) {
  if (folder.children.size() == 0)
    return "";

  const std::string& exposed_name = folder.type_name_pl;

  return std::string(exposed_name + " (" +
                     std::to_string(CalcNumFiltered(folder, filter)) + ")");
}

void OutlinerWidget::DrawNodePic(Child& child, float initial_pos_y,
                                 int icon_size) {
  for (auto* pImg : child.icons_right) {
    ImGui::SameLine();
    ImGui::SetCursorPosY(initial_pos_y);
    drawImageIcon(pImg, icon_size);
  }
}

void OutlinerWidget::AddNewCtxMenu(Child::Folder& folder) {
  if (!CanCreateNew(folder.key))
    return;
  const auto id_str = std::string("MCtx") + folder.key;
  if (ImGui::BeginPopupContextItem(id_str.c_str())) {
    ImGui::TextColored(folder.type_icon_color, "%s",
                       folder.type_icon_pl.c_str());
    ImGui::TextUnformatted((" " + folder.type_name_pl + ": ").c_str());
    ImGui::Separator();

    {
      // set activeModal for an actual gui
      if (ImGui::MenuItem("Add New")) {
        folder.add_new_fn();
        // TODO: Move commit inside?

        // Selection must be reset
        postAddNew();
      }
    }

    ImGui::EndPopup();
  }
}

void OutlinerWidget::DrawFolder(
    NodeFolder& folder, TFilter& mFilter,
    std::optional<std::function<void()>>& activeModal,
    std::string& mActiveClassId) {
  if (folder.children.size() == 0)
    return;

  if (!folder.children[0].has_value() || !folder.children[0]->is_rich)
    return;

  if (folder.key != "ROOT") {
    if (ShouldBeDefaultOpen(folder)) {
      ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    }
    ImGui::PushStyleColor(ImGuiCol_Text, folder.type_icon_color);
    bool opened = ImGui::CollapsingHeader(folder.type_icon_pl.c_str());
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextUnformatted(FormatTitle(folder, &mFilter).c_str());
    AddNewCtxMenu(folder);
    if (!opened) {
      return;
    }
    ImGui::PushID(folder.key.c_str());
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

  int depth = 0;

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
    bool has_child = i + 1 < children.size() &&
                     children[i + 1]->indent > children[i]->indent;

    // Whether or not this node is already selected.
    // Selections from other windows will carry over.
    bool curNodeSelected = isSelected(folder, i);

    const float icon_size = 24.0f * ImGui::GetIO().FontGlobalScale;

    // if (!has_child)
    {
      ImGui::Selectable(std::to_string(i).c_str(), curNodeSelected,
                        ImGuiSelectableFlags_None, {0, icon_size});
      if (ImGui::BeginPopupContextItem(("Ctx" + std::to_string(i)).c_str())) {
        activeModal = [draw_modal = children[i]->draw_modal_fn, f = this]() {
          draw_modal(f);
        };
        ImGui::TextColored(children[i]->type_icon_color, "%s",
                           children[i]->type_icon.c_str());
        ImGui::SameLine();
        ImGui::TextUnformatted(
            (" " + children[i]->type_name + ": " + cur_name).c_str());
        ImGui::Separator();

        children[i]->draw_context_menu_fn(this);

        // set activeModal for an actual gui
        if (folder.delete_child_fn != nullptr && ImGui::MenuItem("Delete")) {
          children[i]->mark_to_delete = true;
        }

        ImGui::EndPopup();
      }

      ImGui::SameLine();
    }

    thereWasAClick = ImGui::IsItemClicked();
    bool focused = ImGui::IsItemFocused();

    u32 flags = ImGuiTreeNodeFlags_DefaultOpen;
    if (!children[i]->is_container && !has_child)
      flags |= ImGuiTreeNodeFlags_Leaf;

    const auto text_size = ImGui::CalcTextSize("T");
    const auto initial_pos_y = ImGui::GetCursorPosY();
    ImGui::SetCursorPosY(initial_pos_y + (icon_size - text_size.y) / 2);

    ImGui::PushStyleColor(ImGuiCol_Text, children[i]->type_icon_color);
    const auto treenode = ImGui::TreeNodeEx(
        std::to_string(i).c_str(), flags, "%s", children[i]->type_icon.c_str());
    if (treenode)
      depth++;
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextUnformatted(cur_name.c_str());
    if (treenode) {
      DrawNodePic(*children[i], initial_pos_y, icon_size);
    }
    ImGui::SetCursorPosY(initial_pos_y + icon_size);
    if (treenode) {
      // NodeDrawer::drawNode(*node.get());

      // drawRecursive(children[i]->folders);
      for (auto& folder : children[i]->folders)
        DrawFolder(folder, mFilter, activeModal, mActiveClassId);

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
    }
    int next;
    if (i + 1 >= children.size()) {
      next = 0;
    } else {
      next = children[i + 1]->indent;
    }
    if (!treenode) {
      int indent = children[i]->indent;
      if (next > indent) {
        ++i;
        while (i < children.size() && children[i]->indent > indent) {
          ++i;
        }
        if (i + 1 >= children.size()) {
          next = 0;
        } else {
          next = children[i]->indent;
        }
        --i;
      }
    }
    for (int j = depth; j > next; --j) {
      ImGui::TreePop();
      --depth;
    }
  }
  assert(depth == 0);
  if (folder.key != "ROOT") {
    ImGui::PopID();
  }

  for (int i = 0; i < children.size(); ++i) {
    if (!children[i]->mark_to_delete)
      continue;
    folder.delete_child_fn(i);
    folder.children.erase(folder.children.begin() + i);
    // Selection must be reset
    postAddNew();
    justSelectedId = -1;
    return;
  }

  // If nothing new was selected, no new processing needs to occur.
  if (justSelectedId == -1)
    return;

  // Unique selection model: No selections of different types.
  // Since we use type-folders, this means only one folder can have selections.
  // We only need to clear the folder of the last active object.
  if (hasActiveSelection() && !mActiveClassId.empty() &&
      mActiveClassId != folder.key) {
    // Invalidate last selection, otherwise SHIFT anchors from the old folder
    // would carry over.
    setActiveSelection(folder, ~0);
    clearSelection();
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
      if (filtered[i] == getActiveSelection(folder))
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
        select(folder, filtered[i]);
    }
  }

  // If the control key was pressed, we add to the selection if necessary.
  if (ctrlPressed && !shiftPressed) {
    // If already selected, no action necessary - just for id update.
    if (!justSelectedAlreadySelected)
      select(folder, justSelectedId);
    else if (thereWasAClick)
      deselect(folder, justSelectedId);
  }

  // Set our last selection index, for shift clicks.
  setActiveSelection(folder, justSelectedId);

  if (!ctrlPressed && !shiftPressed) {
    clearSelection();
  }

  select(folder, justSelectedId);
}

} // namespace riistudio::frontend
