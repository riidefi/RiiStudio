#include "OutlinerWidget.hpp"
#include <frontend/editor/EditorWindow.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

namespace riistudio::frontend {

bool FancyFolderNode(ImVec4 color, const char* icon,
                     std::string_view unitPlural, size_t numEntries) {
  ImGui::PushStyleColor(ImGuiCol_Text, color);
  bool opened = ImGui::CollapsingHeader(icon);
  ImGui::PopStyleColor();
  ImGui::SameLine();
  ImGui::TextUnformatted(
      std::format("{} ({})", unitPlural, numEntries).c_str());
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

  auto id = std::format("{}", child.display_id_relative_to_parent);
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

void OutlinerWidget::AddNewCtxMenu(Node& folder) {
  if (!folder.can_create_new)
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
bool OutlinerWidget::PushFolder(Node& folder, u32 numChildren) {
  if (folder.default_open) {
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
  }
  bool opened =
      FancyFolderNode(folder.type_icon_color, folder.type_icon.c_str(),
                      folder.type_name, numChildren);
  AddNewCtxMenu(folder);
  if (!opened) {
    return false;
  }
  ImGui::PushID(folder.key.c_str());
  return true;
}

bool OutlinerWidget::DrawObject(Node& child, size_t i, bool hasChildren,
                                SelUpdate& update, Node& folder) {
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
    update.mode = imcxx::ContiguousSelection::SELECT_CLICK;
    update.alreadySelected = curNodeSelected;
  } else if (focused && /* selectMode == ContiguousSelection::SELECT_NONE && */
             !curNodeSelected) {
    update.mode = imcxx::ContiguousSelection::SELECT_ARROWKEYS;
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

void OutlinerWidget::DrawFolder(std::vector<Node>&& flat, Node& firstFolder) {
  struct MyIndentedTreeWidget final : private imcxx::IndentedTreeWidget {
  public:
    MyIndentedTreeWidget(OutlinerWidget& outliner, std::vector<Node>& nodes,
                         Node& folder)
        : mOutliner(outliner), mNodes(nodes), mpFolder(&folder) {}

    void Draw() { DrawIndentedTree(); }

  private:
    int Indent(size_t i) const override { return mNodes[i].indent; }
    bool Enabled(size_t i) const override {
      // Filtering is done by the Flatten operation
      return true;
    }
    bool DrawNode(size_t i, size_t filteredIndex, bool hasChild) override {
      auto& node = mNodes[i];
      filtered.push_back(i);
      if (node.nodeType == NODE_OBJECT) {

        // Don't know why but this fixes things
        if (i > 0 && mNodes[i - 1].nodeType == NODE_FOLDER) {
          ImGui::Indent();
        }
        OutlinerWidget::SelUpdate update;
        bool treenode =
            mOutliner.DrawObject(node, i, hasChild, update, *mpFolder);
        if (update.mode != imcxx::ContiguousSelection::SELECT_NONE) {
          selectMode = update.mode;
          justSelectedFilteredIdx = filteredIndex;
          justSelectedAlreadySelected = update.alreadySelected;
        }
        if (!treenode) {
          return false;
        }

        return true;
      } else if (node.nodeType == NODE_FOLDER) {
        mpFolder = &node;
        return mOutliner.PushFolder(*mpFolder, mpFolder->__numChildren);
      }
      return false;
    }
    void CloseNode() override { ImGui::TreePop(); }
    int NumNodes() const override { return mNodes.size(); }

  public:
    // Relative to filter vector.
    std::size_t justSelectedFilteredIdx = -1;
    // Necessary to filter out clicks on already selected items.
    bool justSelectedAlreadySelected = false;
    imcxx::ContiguousSelection::SelectMode selectMode =
        imcxx::ContiguousSelection::SELECT_NONE;
    std::vector<s32> filtered;

  private:
    OutlinerWidget& mOutliner;
    std::vector<Node>& mNodes;
    Node* mpFolder{};
  };

  MyIndentedTreeWidget tree(*this, flat, firstFolder);
  tree.Draw();

  for (int i = 0; i < flat.size(); ++i) {
    if (!flat[i].mark_to_delete)
      continue;
    auto* it = FlattenedTree::getFolderOfObject(flat, i);
    assert(it);
    it->delete_child_fn(&flat[i] - it - 1);
    // Selection must be reset
    postAddNew();
    return;
  }

  // If nothing new was selected, no new processing needs to occur.
  if (tree.selectMode == imcxx::ContiguousSelection::SELECT_NONE)
    return;

// Allow disjoint selections
#if 0
  // Unique selection model: No selections of different types.
  // Since we use type-folders, this means only one folder can have selections.
  // We only need to clear the folder of the last active object.
  Node* folder = nullptr;
  if (hasActiveSelection() && !mActiveClassId.empty()) {
    s32 it = -1;
    for (s32 i = 0; i < flat.size(); ++i) {
      if (isActiveSelection(flat[i])) {
        it = i;
        break;
      }
    }
    assert(it != -1);

    folder = FlattenedTree::getFolderOfObject(flat, it);
    assert(folder);
    if (mActiveClassId != folder->key) {
      // Invalidate last selection, otherwise SHIFT anchors from the old
      // folder would carry over.
      setActiveSelection(nullptr);
      clearSelection();
    }
  }
  if (folder != nullptr) {
    mActiveClassId = folder->key;
  }
#endif

  //
  // Calculation must occur in filtered space to prevent selection
  // of occluded nodes.
  //
  class MyContiguousSelection final : public imcxx::ContiguousSelection {
  public:
    MyContiguousSelection(OutlinerWidget& outliner, std::span<Node> flat,
                          std::vector<s32>&& filtered)
        : mOutliner(outliner), mFlat(flat), mFiltered(std::move(filtered)) {}

    size_t NumNodes() const override { return mFiltered.size(); }
    bool IsActiveSelection(size_t i) const override {
      auto& flat = getFlat(i);
      return mOutliner.isActiveSelection(flat);
    }
    void SetActiveSelection(std::optional<size_t> i) override {
      if (!i.has_value()) {
        mOutliner.setActiveSelection(nullptr);
      } else {
        auto& flat = getFlat(*i);
        assert(flat.nodeType == NODE_OBJECT);
        mOutliner.setActiveSelection(&flat);
      }
    }
    void Select(size_t i) override {
      auto& flat = getFlat(i);
      assert(flat.nodeType == NODE_OBJECT);
      mOutliner.select(flat);
    }
    void Deselect(size_t i) override {
      auto& flat = getFlat(i);
      assert(flat.nodeType == NODE_OBJECT);
      mOutliner.deselect(flat);
    }
    void DeselectAllExcept(std::optional<size_t> i) override {
      if (!i.has_value()) {
        mOutliner.clearSelectionExcept(nullptr);
      } else {
        auto& flat = getFlat(*i);
        assert(flat.nodeType == NODE_OBJECT);
        mOutliner.clearSelectionExcept(&flat);
      }
    }
    bool IsSelectionBarrier(size_t i) const override {
      // For now, folders act as barriers
      auto& flat = getFlat(i);
      return flat.nodeType == NODE_FOLDER;
    }
    bool Selectable(size_t i) const override {
      auto& flat = getFlat(i);
      return flat.nodeType == NODE_OBJECT;
    }

  private:
    OutlinerWidget& mOutliner;
    std::span<Node> mFlat;
    std::vector<s32> mFiltered;

    const Node& getFlat(size_t i) const { return mFlat[mFiltered[i]]; }
    Node& getFlat(size_t i) { return mFlat[mFiltered[i]]; }
  };

  const ImGuiIO& io = ImGui::GetIO();
  MyContiguousSelection contig(*this, flat, std::move(tree.filtered));
  contig.OnUserInput(tree.justSelectedFilteredIdx, io.KeyShift, io.KeyCtrl,
                     tree.justSelectedAlreadySelected, tree.selectMode);
}

} // namespace riistudio::frontend
