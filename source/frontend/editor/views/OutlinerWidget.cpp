#include "OutlinerWidget.hpp"
#include <frontend/editor/EditorWindow.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

#include <frontend/widgets/ContiguousSelection.hpp>
#include <frontend/widgets/IndentedTreeWidget.hpp>

namespace riistudio::frontend {

class FlattenedTree {
public:
  void FromFolder(NodeFolder& folder, int depth,
                  std::invocable<const NodeFolder&> auto&& filterFolder,
                  std::invocable<const Child&> auto&& filterChild) {
    if (!filterFolder(folder)) {
      return;
    }
    auto& node = mNodes.emplace_back(folder);
    node.indent = depth;
    node.nodeType = NODE_FOLDER;
    for (auto& child : folder.children) {
      assert(child.has_value());
      FromChild(*child, depth + 1, filterFolder, filterChild);
    }
    return;
    auto& pop = mNodes.emplace_back();
    pop.indent = depth;
    pop.nodeType = NODE_FOLDER_POP;
  }
  void FromChild(Child& child, int depth,
                 std::invocable<const NodeFolder&> auto&& filterFolder,
                 std::invocable<const Child&> auto&& filterChild) {
    if (!filterChild(child)) {
      return;
    }
    auto& node = mNodes.emplace_back(child);
    node.indent += depth;
    node.nodeType = NODE_OBJECT;
    for (auto& folder : child.folders) {
      FromFolder(folder, depth + 1, filterFolder, filterChild);
    }
  }
  std::vector<Node>&& TakeResult() { return std::move(mNodes); }

  static std::vector<Node>
  Flatten(NodeFolder& root,
          std::invocable<const NodeFolder&> auto&& filterFolder,
          std::invocable<const Child&> auto&& filterChild) {
    FlattenedTree instance;
    instance.FromFolder(root, 0, filterFolder, filterChild);
    return instance.TakeResult();
  }

  static Node* getFolderOfObject(std::ranges::random_access_range auto&& nodes,
                                 std::ptrdiff_t nodeIndex)
    requires std::same_as<std::ranges::range_value_t<decltype(nodes)>, Node>
  {
    assert(nodeIndex < nodes.size());
    while (nodeIndex >= 0 && nodes[nodeIndex].nodeType != NODE_FOLDER) {
      --nodeIndex;
    }
    return nodeIndex >= 0 ? &nodes[nodeIndex] : nullptr;
  }
  static std::optional<std::ptrdiff_t>
  getNextSiblingOrParent(std::ranges::random_access_range auto&& nodes,
                         std::ptrdiff_t nodeIndex)
    requires std::same_as<std::ranges::range_value_t<decltype(nodes)>, Node>
  {
    assert(nodeIndex < nodes.size());
    const auto indent = nodes[nodeIndex].indent;
    while (nodeIndex >= 0 && nodes[nodeIndex].indent > indent) {
      --nodeIndex;
    }
    return nodeIndex >= 0 ? std::optional{nodeIndex} : std::nullopt;
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
u32 ContextFlags = ImGuiWindowFlags_AlwaysAutoResize |
                   ImGuiWindowFlags_NoTitleBar |
                   ImGuiWindowFlags_NoSavedSettings;
bool ShouldContextOpen(
    ImGuiPopupFlags popup_flags = ImGuiPopupFlags_MouseButtonRight) {
  ImGuiContext& g = *GImGui;
  ImGuiWindow* window = g.CurrentWindow;
  if (window->SkipItems)
    return false;
  int mouse_button = (popup_flags & ImGuiPopupFlags_MouseButtonMask_);
  return ImGui::IsMouseReleased(mouse_button) &&
         ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
}

bool FancyObject(Node& child, size_t i, bool hasChild, bool curNodeSelected,
                 bool& thereWasAClick, bool& focused, bool& contextMenu,
                 std::function<void(const lib3d::Texture*, float)> drawIcon) {
  const float icon_size = 24.0f * ImGui::GetIO().FontGlobalScale;

  auto id = std::format("{}", i);
  {
    ImGui::Selectable(id.c_str(), curNodeSelected, ImGuiSelectableFlags_None,
                      {0, icon_size});
    if (ShouldContextOpen()) {
      contextMenu = true;
    }

    ImGui::SameLine();
  }

  thereWasAClick = ImGui::IsItemClicked();
  focused = ImGui::IsItemFocused();

  u32 flags = ImGuiTreeNodeFlags_DefaultOpen;
  if (!child.is_container && !hasChild)
    flags |= ImGuiTreeNodeFlags_Leaf;

  const auto text_size = ImGui::CalcTextSize("T");
  const auto initial_pos_y = ImGui::GetCursorPosY();
  ImGui::SetCursorPosY(initial_pos_y + (icon_size - text_size.y) / 2.0f);

  ImGui::PushStyleColor(ImGuiCol_Text, child.type_icon_color);
  const auto treenode =
      ImGui::TreeNodeEx(id.c_str(), flags, "%s", child.type_icon.c_str());
  ImGui::PopStyleColor();
  ImGui::SameLine();
  ImGui::TextUnformatted(child.public_name.c_str());
  if (treenode) {
    for (auto* pImg : child.icons_right) {
      ImGui::SameLine();
      ImGui::SetCursorPosY(initial_pos_y);
      drawIcon(pImg, icon_size);
    }
  }
  ImGui::SetCursorPosY(initial_pos_y + icon_size);
  return treenode;
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
bool FilterChild(const Child& child, TFilter& filter) {
  if (!child.is_container && !filter.test(child.public_name))
    return false;

  // We rely on rich type info to infer icons
  if (!child.is_rich)
    return false;

  return true;
}

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
    return mOutliner.isActiveSelection(*mFolder.children[mFiltered[i]]);
  }
  void SetActiveSelection(std::optional<size_t> i) override {
    if (!i.has_value())
      mOutliner.setActiveSelection(nullptr);
    else
      mOutliner.setActiveSelection(&*mFolder.children[mFiltered[*i]]);
  }
  void Select(size_t i) override {
    mOutliner.select(*mFolder.children[mFiltered[i]]);
  }
  void Deselect(size_t i) override {
    mOutliner.deselect(*mFolder.children[mFiltered[i]]);
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
                       TFilter& filter)
      : mOutliner(outliner), mFolder(folder), mFilter(filter) {}

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
    return FilterChild(*child, mFilter);
  }
  bool DrawNode(size_t i, size_t filteredIndex, bool hasChild) override {
    if (!mFolder.children[i].has_value()) {
      return false;
    }
    filtered.push_back(i);
    Child& child = *mFolder.children[i];

    OutlinerWidget::SelUpdate update;
    bool treenode = mOutliner.DrawObject(child, i, hasChild, update, mFolder);
    if (update.mode != ContiguousSelection::SELECT_NONE) {
      selectMode = update.mode;
      justSelectedFilteredIdx = filteredIndex;
      justSelectedAlreadySelected = update.alreadySelected;
    }
    if (!treenode) {
      return false;
    }
    for (auto& folder : child.folders)
      mOutliner.DrawFolder(folder, mFilter);

    return true;
  }
  void CloseNode() override { ImGui::TreePop(); }
  int NumNodes() const override { return mFolder.children.size(); }

public:
  std::vector<int> filtered;
  // Relative to filter vector.
  std::size_t justSelectedFilteredIdx = -1;
  // Necessary to filter out clicks on already selected items.
  bool justSelectedAlreadySelected = false;
  ContiguousSelection::SelectMode selectMode = ContiguousSelection::SELECT_NONE;

private:
  OutlinerWidget& mOutliner;
  NodeFolder& mFolder;
  TFilter& mFilter;
};

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

void OutlinerWidget::DrawNodePic(Node& child, float initial_pos_y,
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

bool OutlinerWidget::DrawObject(Child& child, size_t i, bool hasChildren,
                                SelUpdate& update, NodeFolder& folder) {
  // Whether or not this node is already selected.
  // Selections from other windows will carry over.
  bool curNodeSelected = isSelected(child);

  bool contextMenu = false;
  bool thereWasAClick = false;
  bool focused;
  bool treenode = FancyObject(
      child, i, hasChildren, curNodeSelected, thereWasAClick, focused,
      contextMenu, [&](auto* img, float size) { drawImageIcon(img, size); });
  if (thereWasAClick) {
    update.mode = ContiguousSelection::SELECT_CLICK;
    update.alreadySelected = curNodeSelected;
  } else if (focused && /* selectMode == ContiguousSelection::SELECT_NONE && */
             !curNodeSelected) {
    update.mode = ContiguousSelection::SELECT_ARROWKEYS;
    update.alreadySelected = curNodeSelected;
  }
  auto id = std::format("Ctx {}", i);
  if (contextMenu) {
    ImGui::OpenPopup(id.c_str());
    setActiveModal(&child);
  }
  if (ImGui::BeginPopup(id.c_str(), ContextFlags)) {
    ImGui::TextColored(child.type_icon_color, "%s", child.type_icon.c_str());
    ImGui::SameLine();
    ImGui::TextUnformatted(
        (" " + child.type_name + ": " + child.public_name).c_str());
    ImGui::Separator();

    child.draw_context_menu_fn(this);

    // set activeModal for an actual gui
    if (folder.delete_child_fn != nullptr && ImGui::MenuItem("Delete")) {
      child.mark_to_delete = true;
    }

    ImGui::EndPopup();
  }
  return treenode;
}

void OutlinerWidget::DrawFolder(NodeFolder& folder, TFilter& mFilter) {
  if (!FilterFolder(folder))
    return;

  if (!PushFolder(folder, mFilter)) {
    return;
  }

  MyIndentedTreeWidget tree(*this, folder, mFilter);
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
  if (tree.selectMode == ContiguousSelection::SELECT_NONE)
    return;

  // Unique selection model: No selections of different types.
  // Since we use type-folders, this means only one folder can have selections.
  // We only need to clear the folder of the last active object.
  if (hasActiveSelection() && !mActiveClassId.empty() &&
      mActiveClassId != folder.key) {
    // Invalidate last selection, otherwise SHIFT anchors from the old folder
    // would carry over.
    setActiveSelection(nullptr);
    clearSelection();
  }
  mActiveClassId = folder.key;

  const ImGuiIO& io = ImGui::GetIO();
  MyContiguousSelection contig(*this, folder, tree.filtered);
  contig.OnUserInput(tree.justSelectedFilteredIdx, io.KeyShift, io.KeyCtrl,
                     tree.justSelectedAlreadySelected, tree.selectMode);
}

} // namespace riistudio::frontend
