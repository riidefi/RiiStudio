#include "Outliner.hpp"
#include <core/kpi/RichNameManager.hpp>     // kpi::RichNameManager
#include <frontend/editor/EditorWindow.hpp> // EditorWindow
#include <frontend/editor/StudioWindow.hpp> // StudioWindow
#include <plugins/gc/Export/Material.hpp>   // libcube::IGCMaterial
//#include <regex>                          // std::regex_search
#include <vendor/fa5/IconsFontAwesome5.h> // (const char*)ICON_FA_SEARCH

#include <core/kpi/ActionMenu.hpp> // kpi::ActionMenuManager

#include <plugins/gc/Export/Scene.hpp>

bool IsAdvancedMode();

namespace riistudio::frontend {

std::string NameObject(const kpi::IObject* obj, int i) {
  std::string base = obj->getName();

  if (base == "TODO") {
    const auto rich = kpi::RichNameManager::getInstance().getRich(obj);
    if (rich.hasEntry())
      base = rich.getNameSingular() + " #" + std::to_string(i);
  }

  std::string extra;

  if (IsAdvancedMode()) {
    const libcube::IGCMaterial* mat =
        dynamic_cast<const libcube::IGCMaterial*>(obj);
    if (mat != nullptr) {
      extra = "  [#Stages=" +
              std::to_string(mat->getMaterialData().mStages.size()) +
              ",#Samplers=" +
              std::to_string(mat->getMaterialData().samplers.size()) + "]";
    }
  }

  return base + extra;
}

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

struct Child {
public:
  kpi::IObject* obj;

private:
  // kpi::INode* node;

public:
  // Formatted, [#Stages=n]
  std::string public_name;

  // Rich name (Icon+Name of type, singular/plural)
  bool is_rich = false;
  kpi::RichNameManager::EntryDelegate rich;

  struct Folder {
    std::vector<std::optional<Child>> children;

    // typename e.g. `class riistudio::g3d::Model`
    std::string key;

    // Still here for selection state
    kpi::ICollection& sampler;
  };

  std::vector<Folder> folders;

  // Potentially has sub-folders
  bool is_container_ = false;

  bool is_container() const {
    // return node != nullptr;
    return is_container_;
  }
};

//! @brief Return the number of resources in the source that pass the filter.
//!
static std::size_t CalcNumFiltered(const Child::Folder& folder,
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

//! @brief Format the title in the "<header> (<number of resources>)" format.
//!
static std::string FormatTitle(const Child::Folder& folder,
                               const TFilter* filter) {
  if (folder.children.size() == 0)
    return "";
  const auto rich = folder.children[0]->rich;
  const std::string icon_plural = rich.getIconPlural();
  const std::string exposed_name = rich.getNamePlural();
  return std::string(icon_plural + "  " + exposed_name + " (" +
                     std::to_string(CalcNumFiltered(folder, filter)) + ")");
}

struct GenericCollectionOutliner : public StudioWindow {
  GenericCollectionOutliner(kpi::INode& host, kpi::IObject*& active,
                            EditorWindow& ed);

  void drawFolder(Child::Folder& folder) noexcept;

  void drawRecursive(std::vector<Child::Folder> folders) noexcept;

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

// For models and bones we disable "add new" functionality for some reason
static bool CanCreateNew(std::string_view key) {
  return !key.ends_with("Model") && !key.ends_with("Bone");
}

std::vector<std::optional<Child>> GetChildren(kpi::ICollection& sampler);
auto GetGChildren(kpi::INode* node) {
  std::vector<Child::Folder> grandchildren;

  if (node != nullptr) {
    for (int i = 0; i < node->numFolders(); ++i) {
      auto children = GetChildren(*node->folderAt(i));
      grandchildren.push_back(Child::Folder{.children = children,
                                            .key = node->idAt(i),
                                            .sampler = *node->folderAt(i)});
    }
  }

  return grandchildren;
}

std::vector<std::optional<Child>> GetChildren(kpi::ICollection& sampler) {
  std::vector<std::optional<Child>> children(sampler.size());
  for (int i = 0; i < children.size(); ++i) {
    auto* node = dynamic_cast<kpi::INode*>(sampler.atObject(i));
    auto public_name = NameObject(sampler.atObject(i), i);
    auto rich =
        kpi::RichNameManager::getInstance().getRich(sampler.atObject(i));
    bool is_rich = kpi::RichNameManager::getInstance()
                       .getRich(sampler.atObject(i))
                       .hasEntry();

    auto grandchildren = GetGChildren(node);

    children[i] = Child{.obj = sampler.atObject(i),
                        // .node = node,
                        .public_name = public_name,
                        .is_rich = is_rich,
                        .rich = rich,
                        .folders = grandchildren,
                        .is_container_ = node != nullptr};
  }

  return children;
}

void DrawNodePic(EditorWindow& ed, kpi::IObject& nodeAt,
                 const float& initial_pos_y, const int& icon_size) {
  lib3d::Texture* tex = dynamic_cast<lib3d::Texture*>(&nodeAt);
  libcube::IGCMaterial* mat = dynamic_cast<libcube::IGCMaterial*>(&nodeAt);
  libcube::Scene* scn =
      nodeAt.childOf && nodeAt.childOf->childOf
          ? dynamic_cast<libcube::Scene*>(nodeAt.childOf->childOf)
          : nullptr;
  if (mat != nullptr) {
    for (int s = 0; s < mat->getMaterialData().samplers.size(); ++s) {
      auto& sampl = mat->getMaterialData().samplers[s];
      const lib3d::Texture* curImg = mat->getTexture(*scn, sampl.mTexture);
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

void AddNewCtxMenu(EditorWindow& ed, Child::Folder& folder) {
  if (CanCreateNew(folder.key)) {
    const auto local_id = reinterpret_cast<u64>(&folder.sampler);
    const auto id_str = std::string("MCtx") + std::to_string(local_id);
    if (ImGui::BeginPopupContextItem(id_str.c_str())) {
      ImGui::TextUnformatted((folder.children[0]->rich.getIconPlural() + " " +
                              folder.children[0]->rich.getNamePlural() + ": ")
                                 .c_str());
      ImGui::Separator();

      {
        // set activeModal for an actual gui
        if (ImGui::MenuItem("Add New")) {
          folder.sampler.add();
          ed.getDocument().commit();
        }
      }

      ImGui::EndPopup();
    }
  }
}

void GenericCollectionOutliner::drawFolder(Child::Folder& folder) noexcept {
  if (folder.children.size() == 0)
    return;

  if (!folder.children[0].has_value() || !folder.children[0]->is_rich)
    return;

  ImGui::SetNextItemOpen(true, ImGuiCond_Once);
  const bool opened = ImGui::TreeNode(FormatTitle(folder, &mFilter).c_str());
  AddNewCtxMenu(ed, folder);
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

  auto& children = folder.children;

  // Draw the tree
  for (int i = 0; i < children.size(); ++i) {
    if (!children[i].has_value()) {
      continue;
    }

    auto& nodeAt = *children[i]->obj;
    std::string cur_name = children[i]->public_name;

    if (!children[i]->is_container() && !mFilter.test(children[i]->public_name))
      continue;

    // TODO: Do we still need this?
    if (!children[i]->is_rich)
      continue;

    filtered.push_back(i);

    // Whether or not this node is already selected.
    // Selections from other windows will carry over.
    bool curNodeSelected = folder.sampler.isSelected(i);

    const int icon_size = 24;

    ImGui::Selectable(std::to_string(i).c_str(), curNodeSelected,
                      ImGuiSelectableFlags_None, {0, icon_size});
    if (ImGui::BeginPopupContextItem(("Ctx" + std::to_string(i)).c_str())) {
      activeModal = &nodeAt;
      ImGui::TextUnformatted((children[i]->rich.getIconSingular() + " " +
                              children[i]->rich.getNameSingular() + ": " +
                              cur_name)
                                 .c_str());
      ImGui::Separator();

      {
        if (kpi::ActionMenuManager::get().drawContextMenus(nodeAt))
          ed.getDocument().commit();
      }

      ImGui::EndPopup();
    }

    ImGui::SameLine();

    thereWasAClick = ImGui::IsItemClicked();
    bool focused = ImGui::IsItemFocused();

    u32 flags = ImGuiTreeNodeFlags_DefaultOpen;
    if (!children[i]->is_container())
      flags |= ImGuiTreeNodeFlags_Leaf;

    const auto text_size = ImGui::CalcTextSize("T");
    const auto initial_pos_y = ImGui::GetCursorPosY();
    ImGui::SetCursorPosY(initial_pos_y + (icon_size - text_size.y) / 2);

    const auto treenode = ImGui::TreeNodeEx(
        std::to_string(i).c_str(), flags, "%s %s",
        children[i]->rich.getIconSingular().c_str(), cur_name.c_str());

    if (treenode) {
      DrawNodePic(ed, nodeAt, initial_pos_y, icon_size);
    }
    ImGui::SetCursorPosY(initial_pos_y + icon_size);
    if (treenode) {
      // NodeDrawer::drawNode(*node.get());

      drawRecursive(children[i]->folders);

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
      mActive->collectionOf != &folder.sampler) {
    // Invalidate last selection, otherwise SHIFT anchors from the old folder
    // would carry over.
    folder.sampler.setActiveSelection(~0);
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
      if (filtered[i] == folder.sampler.getActiveSelection())
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
        folder.sampler.select(filtered[i]);
    }
  }

  // If the control key was pressed, we add to the selection if necessary.
  if (ctrlPressed && !shiftPressed) {
    // If already selected, no action necessary - just for id update.
    if (!justSelectedAlreadySelected)
      folder.sampler.select(justSelectedId);
    else if (thereWasAClick)
      folder.sampler.deselect(justSelectedId);
  } else if (!ctrlPressed && !shiftPressed &&
             (thereWasAClick ||
              justSelectedId != folder.sampler.getActiveSelection())) {
    // Replace selection
    folder.sampler.clearSelection();
    folder.sampler.select(justSelectedId);
  }

  // Set our last selection index, for shift clicks.
  folder.sampler.setActiveSelection(justSelectedId);
  mActive =
      folder.sampler.atObject(justSelectedId); // TODO -- removing the active
                                               // node will cause issues here
}

void GenericCollectionOutliner::drawRecursive(
    std::vector<Child::Folder> folders) noexcept {
  for (auto& f : folders)
    drawFolder(f);
}

void GenericCollectionOutliner::draw_() noexcept {
  // activeModal = nullptr;
  mFilter.Draw();
  auto& _h = (kpi::INode&)mHost;
  auto children = GetGChildren(&_h);
  drawRecursive(children);

  if (activeModal != nullptr)
    if (kpi::ActionMenuManager::get().drawModals(*activeModal, &ed))
      ed.getDocument().commit();
}

std::unique_ptr<StudioWindow>
MakeOutliner(kpi::INode& host, kpi::IObject*& active, EditorWindow& ed) {
  return std::make_unique<GenericCollectionOutliner>(host, active, ed);
}

} // namespace riistudio::frontend
