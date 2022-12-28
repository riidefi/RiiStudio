#pragma once

#include <functional> // std::function
#include <optional>   // std::optional
#include <string>     // std::string
#include <vector>     // std::vector

#include <core/3d/Texture.hpp> // lib3d::Texture*
#include <core/kpi/Node2.hpp>  // kpi::IObject*

#include <imgui/imgui.h> // ImGuiTextFilter

namespace riistudio::frontend {

struct OutlinerWidget;

struct Child {
public:
  // Only here for EditorWindow::mActive
  kpi::IObject* obj;

  // Formatted, [#Stages=n]
  std::string public_name;

  // Rich name (Icon+Name of type, singular/plural)
  //
  // Previously stored a RichName, now use type_icon, type_name
  //
  // If folder[0]->is_rich is NOT set, the folder will not be displayed.
  bool is_rich = false;

  // Icons on the left of the text
  //
  // RichName::getIconSingular()
  //
  // Images can be used, with a special character that indexes the font sheet
  // See: FontAwesome.h
  std::string type_icon = "(?)";
  ImVec4 type_icon_color = {1.0f, 1.0f, 1.0f, 1.0f};

  // Used by context menu
  //
  // RichName::getNameSingular()
  //
  std::string type_name = "Unknown Thing";

  // Icons on the right of the text
  std::vector<const lib3d::Texture*> icons_right;

  struct Folder {
    std::vector<std::optional<Child>> children;

    // typename e.g. `class riistudio::g3d::Model`
    std::string key;

    // TODO: This function can produce stale references
    std::function<void()> add_new_fn;
    std::function<void(size_t)> delete_child_fn;

    // RichName::getIconPlural()
    std::string type_icon_pl = "(?)";

    ImVec4 type_icon_color = {1.0f, 1.0f, 1.0f, 1.0f};

    // RichName::getIconPlural()
    std::string type_name_pl = "Unknown Things";
  };

  std::vector<Folder> folders;

  // Potentially has sub-folders
  //
  // When this attribute is set, this object will always be shown even when the
  // name fails the search filter
  bool is_container = false;

  // This should only increase or decrease by one per node
  // Not the most amazing way to implement recursive folders, since it requires
  // DFS layout of nodes.
  int indent = 0;

  std::function<void(OutlinerWidget*)> draw_context_menu_fn;
  // TODO: Replace with modal stack
  std::function<void(OutlinerWidget*)> draw_modal_fn;

  // TODO
  bool mark_to_delete = false;
};

using NodeFolder = Child::Folder;

struct ImTFilter : public ImGuiTextFilter {
  bool test(const std::string& str) const noexcept {
    return PassFilter(str.c_str());
  }
};
using TFilter = ImTFilter;

struct OutlinerWidget {
  std::size_t CalcNumFiltered(const NodeFolder& folder, const TFilter* filter);
  std::string FormatTitle(const NodeFolder& folder, const TFilter* filter);

  void DrawNodePic(Child& child, float initial_pos_y, int icon_size);

  void DrawFolder(NodeFolder& folder, TFilter& mFilter,
                  std::optional<std::function<void()>>& activeModal,
                  std::string& mActiveClassId);

  virtual bool isSelected(NodeFolder& f, size_t i) const = 0;
  virtual void select(NodeFolder& f, size_t i) = 0;
  virtual void deselect(NodeFolder& f, size_t i) = 0;
  virtual bool hasActiveSelection() const = 0;
  virtual size_t getActiveSelection(NodeFolder& nodes) const = 0;
  virtual void setActiveSelection(NodeFolder& nodes, size_t index) = 0;

  virtual void postAddNew() = 0;
  virtual void drawImageIcon(const riistudio::lib3d::Texture* pImg,
                             int icon_size) = 0;
  virtual void clearSelection() = 0;

private:
  void AddNewCtxMenu(Child::Folder& folder);
};

} // namespace riistudio::frontend
