#pragma once

#include <functional> // std::function
#include <optional>   // std::optional
#include <string>     // std::string
#include <vector>     // std::vector

#include <core/3d/Texture.hpp> // lib3d::Texture*
#include <core/kpi/Node2.hpp>  // kpi::IObject*

#include <imgui/imgui.h> // ImGuiTextFilter

#include <frontend/widgets/ContiguousSelection.hpp>

namespace riistudio::frontend {

class OutlinerWidget;

enum NodeType {
  NODE_OBJECT,
  NODE_FOLDER,
};

struct Node {
  NodeType nodeType = NODE_OBJECT;
  // This should only increase or decrease by one per node
  //
  // FOR NOW, THIS IS RELATIVE TO THE CHILD NODE
  int indent = 0;

  // Images can be used, with a special character that indexes the font sheet
  // See: FontAwesome.h
  std::string type_icon = "(?)";
  ImVec4 type_icon_color = {1.0f, 1.0f, 1.0f, 1.0f};
  // RichName::getNameSingular()
  std::string type_name = "Unknown Thing";

  ///////////////////////////////////////////////////////
  // FOLDER ONLY
  ///////////////////////////////////////////////////////

  // TODO: These functions can produce stale references
  std::function<void()> add_new_fn;
  std::function<void(size_t)> delete_child_fn;
  // typename e.g. `class riistudio::g3d::Model`
  std::string key;
  bool default_open = true;
  // TODO: MOVE OUT
  bool can_create_new = true;

  ///////////////////////////////////////////////////////
  // FILE ONLY (for now)
  ///////////////////////////////////////////////////////

  // Not supported by folders (yet)
  bool mark_to_delete = false;

  // Icons on the right of the text
  std::vector<const lib3d::Texture*> icons_right;

  // These are not supported by folders (yet)
  std::function<void(OutlinerWidget*)> draw_context_menu_fn;
  // TODO: Replace with modal stack
  std::function<void(OutlinerWidget*)> draw_modal_fn;

  // Formatted, [#Stages=n]
  std::string public_name;

  // Only here for EditorWindow::mActive
  kpi::IObject* obj;

  // Potentially has sub-folders
  //
  // When this attribute is set, this object will always be shown even when the
  // name fails the search filter
  bool is_container = false;

  // Rich name (Icon+Name of type, singular/plural)
  //
  // Previously stored a RichName, now use type_icon, type_name
  //
  // If folder[0]->is_rich is NOT set, the folder will not be displayed.
  bool is_rich = false;

  ///////////////////////////////////////////////////////
  // INTERNAL
  ///////////////////////////////////////////////////////
  int __numChildren = 0;
};

struct Child : public Node {
public:
  struct Folder : public Node {
    std::vector<std::optional<Child>> children;
  };
  std::vector<Folder> folders;
};

using NodeFolder = Child::Folder;

struct ImTFilter : public ImGuiTextFilter {
  bool test(const std::string& str) const noexcept {
    return PassFilter(str.c_str());
  }
};
using TFilter = ImTFilter;

class OutlinerWidget {
public:
  void DrawFolder(NodeFolder& folder, TFilter& mFilter);

  virtual bool isSelected(const Node& n) const = 0;
  virtual void select(const Node& n) = 0;
  virtual void deselect(const Node& n) = 0;
  virtual bool hasActiveSelection() const = 0;
  virtual bool isActiveSelection(const Node& n) const = 0;
  virtual void setActiveSelection(const Node* node = nullptr) = 0;
  virtual void postAddNew() = 0;
  virtual void drawImageIcon(const riistudio::lib3d::Texture* pImg,
                             int icon_size) = 0;
  virtual void clearSelection() = 0;
  virtual void setActiveModal(const Node* = nullptr) = 0;
  struct SelUpdate {
    ContiguousSelection::SelectMode mode = ContiguousSelection::SELECT_NONE;
    bool alreadySelected = false;
  };
  bool DrawObject(Node& child, size_t i, bool hasChildren, SelUpdate& update,
                  Node& folder);

private:
  void AddNewCtxMenu(Node& folder);

  bool PushFolder(Node& folder, u32 numChildren);
  // Describes the type of mActive, useful for differentiating type-disjoint
  // multiselections.
  std::string mActiveClassId;
};

} // namespace riistudio::frontend
