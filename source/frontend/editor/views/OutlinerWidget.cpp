#include "OutlinerWidget.hpp"
#include <frontend/editor/EditorWindow.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

#include <frontend/widgets/ContiguousSelection.hpp>
#include <frontend/widgets/IndentedTreeWidget.hpp>

namespace riistudio::frontend {

//
// Calculation must occur in filtered space to prevent selection
// of occluded nodes.
//
class MyContiguousSelection : public ContiguousSelection {
public:
  MyContiguousSelection(OutlinerWidget& outliner, NodeFolder& folder,
                        std::vector<int>& filtered)
      : mOutliner(outliner), mFolder(folder), mFiltered(filtered) {}

  size_t NumNodes() const override { return mFiltered.size(); }
  bool IsActiveSelection(size_t i) const override {
    return mFiltered[i] == mOutliner.getActiveSelection(mFolder);
  }
  void SetActiveSelection(size_t i) override {
    mOutliner.setActiveSelection(mFolder, mFiltered[i]);
  }
  void Select(size_t i) override { mOutliner.select(mFolder, mFiltered[i]); }
  void Deselect(size_t i) override {
    mOutliner.deselect(mFolder, mFiltered[i]);
  }
  void DeselectAll() override { mOutliner.clearSelection(); }

private:
  OutlinerWidget& mOutliner;
  NodeFolder& mFolder;
  std::vector<int> mFiltered;
};

//
// This only handles elements of a single folder currently.
//
struct MyIndentedTreeWidget : private IndentedTreeWidget {
public:
  MyIndentedTreeWidget(OutlinerWidget& outliner, NodeFolder& folder,
                       TFilter& filter,
                       std::optional<std::function<void()>>& activeModal_,
                       std::string& activeClassId_)
      : mOutliner(outliner), mFolder(folder), mFilter(filter),
        activeModal(activeModal_), mActiveClassId(activeClassId_) {}

  void Draw() {
    filtered.clear();
    DrawIndentedTree();
  }

private:
  int Indent(size_t i) const override {
    auto& child = mFolder.children[i];
    assert(child.has_value());
    return child->indent - mFolder.indent;
  }
  bool Enabled(size_t i) const override {
    auto& child = mFolder.children[i];
    assert(child.has_value());
    if (!child->is_container && !mFilter.test(child->public_name))
      return false;

    // We rely on rich type info to infer icons
    if (!child->is_rich)
      return false;

    return true;
  }
  bool DrawNode(size_t i, size_t filteredIndex, bool hasChild) override {
    if (!mFolder.children[i].has_value()) {
      return false;
    }
    filtered.push_back(i);
    Child& child = *mFolder.children[i];

    std::string cur_name = child.public_name;
    // Whether or not this node is already selected.
    // Selections from other windows will carry over.
    bool curNodeSelected = mOutliner.isSelected(mFolder, i);

    const float icon_size = 24.0f * ImGui::GetIO().FontGlobalScale;

    {
      ImGui::Selectable(std::to_string(i).c_str(), curNodeSelected,
                        ImGuiSelectableFlags_None, {0, icon_size});
      if (ImGui::BeginPopupContextItem(("Ctx" + std::to_string(i)).c_str())) {
        activeModal = [draw_modal = child.draw_modal_fn, f = &mOutliner]() {
          draw_modal(f);
        };
        ImGui::TextColored(child.type_icon_color, "%s",
                           child.type_icon.c_str());
        ImGui::SameLine();
        ImGui::TextUnformatted(
            (" " + child.type_name + ": " + cur_name).c_str());
        ImGui::Separator();

        child.draw_context_menu_fn(&mOutliner);

        // set activeModal for an actual gui
        if (mFolder.delete_child_fn != nullptr && ImGui::MenuItem("Delete")) {
          child.mark_to_delete = true;
        }

        ImGui::EndPopup();
      }

      ImGui::SameLine();
    }

    thereWasAClick = ImGui::IsItemClicked();
    bool focused = ImGui::IsItemFocused();

    u32 flags = ImGuiTreeNodeFlags_DefaultOpen;
    if (!child.is_container && !hasChild)
      flags |= ImGuiTreeNodeFlags_Leaf;

    const auto text_size = ImGui::CalcTextSize("T");
    const auto initial_pos_y = ImGui::GetCursorPosY();
    ImGui::SetCursorPosY(initial_pos_y + (icon_size - text_size.y) / 2);

    ImGui::PushStyleColor(ImGuiCol_Text, child.type_icon_color);
    const auto treenode = ImGui::TreeNodeEx(std::to_string(i).c_str(), flags,
                                            "%s", child.type_icon.c_str());
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextUnformatted(cur_name.c_str());
    if (treenode) {
      mOutliner.DrawNodePic(child, initial_pos_y, icon_size);
    }
    ImGui::SetCursorPosY(initial_pos_y + icon_size);
    if (treenode) {
      for (auto& folder : child.folders)
        mOutliner.DrawFolder(folder, mFilter, activeModal, mActiveClassId);

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
    return treenode;
  }
  void CloseNode() override { ImGui::TreePop(); }
  int NumNodes() const override { return mFolder.children.size(); }

public:
  std::vector<int> filtered;
  int justSelectedId = -1;
  // Relative to filter vector.
  std::size_t justSelectedFilteredIdx = -1;
  // Necessary to filter out clicks on already selected items.
  bool justSelectedAlreadySelected = false;
  // Prevent resetting when SHIFT is unpressed with arrow keys.
  bool thereWasAClick = false;

private:
  OutlinerWidget& mOutliner;
  NodeFolder& mFolder;
  TFilter& mFilter;
  // Cringe
  std::optional<std::function<void()>>& activeModal;
  std::string& mActiveClassId;
};

class FlattenedTree {
public:
  void FromFolder(NodeFolder& folder, int depth, auto filterFolder) {
    if (!filterFolder(folder)) {
      return;
    }
    auto& node = mNodes.emplace_back(folder);
    node.indent = depth;
    node.nodeType = NODE_FOLDER;
    for (auto& child : folder.children) {
      assert(child.has_value());
      FromChild(*child, depth + 1, filterFolder);
    }
  }
  void FromChild(Child& child, int depth, auto filterFolder) {
    auto& node = mNodes.emplace_back(child);
    node.indent += depth;
    node.nodeType = NODE_OBJECT;
    for (auto& folder : child.folders) {
      FromFolder(folder, depth + 1, filterFolder);
    }
  }
  std::vector<Node>&& TakeResult() { return std::move(mNodes); }

  static std::vector<Node> Flatten(NodeFolder& root, auto filterFolder) {
    FlattenedTree instance;
    instance.FromFolder(root, 0, filterFolder);
    return instance.TakeResult();
  }

private:
  std::vector<Node> mNodes;
};

std::string FormatTitleStr(std::string_view unit_plural, size_t numUnder) {
  if (numUnder == 0)
    return "";

  return std::format("{} ({})", unit_plural, numUnder);
}
bool FancyFolderNode(ImVec4 color, const char* icon,
                     std::string_view unitPlural, size_t numEntries) {
  ImGui::PushStyleColor(ImGuiCol_Text, color);
  bool opened = ImGui::CollapsingHeader(icon);
  ImGui::PopStyleColor();
  ImGui::SameLine();
  ImGui::TextUnformatted(FormatTitleStr(unitPlural, numEntries).c_str());
  return opened;
}

// For models and bones we disable "add new" functionality for some reason
static bool CanCreateNew(std::string_view key) {
  return !key.ends_with("Model") && !key.ends_with("Bone");
}
bool ShouldBeDefaultOpen(const Node& folder) {
  // Polygons
  if (folder.type_icon == (const char*)ICON_FA_DRAW_POLYGON)
    return false;
  // Vertex Colors
  if (folder.type_icon == (const char*)ICON_FA_BRUSH)
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

void OutlinerWidget::DrawNodePic(Child& child, float initial_pos_y,
                                 int icon_size) {
  for (auto* pImg : child.icons_right) {
    ImGui::SameLine();
    ImGui::SetCursorPosY(initial_pos_y);
    drawImageIcon(pImg, icon_size);
  }
}

void OutlinerWidget::AddNewCtxMenu(Node& folder) {
  if (!CanCreateNew(folder.key))
    return;
  const auto id_str = std::string("MCtx") + folder.key;
  if (ImGui::BeginPopupContextItem(id_str.c_str())) {
    ImGui::TextColored(folder.type_icon_color, "%s", folder.type_icon.c_str());
    ImGui::TextUnformatted((" " + folder.type_name + ": ").c_str());
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
bool OutlinerWidget::PushFolder(Child::Folder& folder, TFilter& filter) {
  if (folder.key == "ROOT")
    return true;
  if (ShouldBeDefaultOpen(folder)) {
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
  }
  bool opened =
      FancyFolderNode(folder.type_icon_color, folder.type_icon.c_str(),
                      folder.type_name, CalcNumFiltered(folder, &filter));
  AddNewCtxMenu(folder);
  if (!opened) {
    return false;
  }
  ImGui::PushID(folder.key.c_str());
  return true;
}
void OutlinerWidget::PopFolder(Child::Folder& folder) {
  if (folder.key != "ROOT") {
    ImGui::PopID();
  }
}

bool FilterFolder(const NodeFolder& folder) {
  // Do not show empty folders
  if (folder.children.size() == 0)
    return false;

  // Do not display folders without rich type info (at the first child)
  if (!folder.children[0].has_value() || !folder.children[0]->is_rich)
    return false;

  return true;
}

void OutlinerWidget::DrawFolder(
    NodeFolder& folder, TFilter& mFilter,
    std::optional<std::function<void()>>& activeModal,
    std::string& mActiveClassId) {
  if (!FilterFolder(folder))
    return;

  if (!PushFolder(folder, mFilter)) {
    return;
  }

  MyIndentedTreeWidget tree(*this, folder, mFilter, activeModal,
                            mActiveClassId);
  tree.Draw();

  PopFolder(folder);

  for (int i = 0; i < folder.children.size(); ++i) {
    if (!folder.children[i]->mark_to_delete)
      continue;
    folder.delete_child_fn(i);
    folder.children.erase(folder.children.begin() + i);
    // Selection must be reset
    postAddNew();
    return;
  }

  // If nothing new was selected, no new processing needs to occur.
  if (tree.justSelectedId == -1)
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

  const ImGuiIO& io = ImGui::GetIO();
  MyContiguousSelection contig(*this, folder, tree.filtered);
  contig.OnUserInput(tree.justSelectedFilteredIdx, io.KeyShift, io.KeyCtrl,
                     tree.justSelectedAlreadySelected, tree.thereWasAClick);
}

} // namespace riistudio::frontend
