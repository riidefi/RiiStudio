#pragma once

#include <functional> // std::function
#include <optional>   // std::optional
#include <string>     // std::string
#include <vector>     // std::vector

#include <core/3d/Texture.hpp> // lib3d::Texture*
#include <core/kpi/Node2.hpp>  // kpi::IObject*

#include <imgui/imgui.h> // ImGuiTextFilter

namespace riistudio::frontend {

class EditorWindow;

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

    // RichName::getIconPlural()
    std::string type_name_pl = "Unknown Things";
  };

  std::vector<Folder> folders;

  // Potentially has sub-folders
  //
  // When this attribute is set, this object will always be shown even when the
  // name fails the search filter
  bool is_container = false;

  std::function<void(EditorWindow&)> draw_context_menu_fn;
  // TODO: Replace with modal stack
  std::function<void(EditorWindow&)> draw_modal_fn;

  // TODO
  bool mark_to_delete = false;
};

using NodeFolder = Child::Folder;

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
std::size_t CalcNumFiltered(const NodeFolder& folder, const TFilter* filter);

//! @brief Format the title in the "<header> (<number of resources>)" format.
//!
std::string FormatTitle(const NodeFolder& folder, const TFilter* filter);

void DrawNodePic(EditorWindow& ed, Child& child, float initial_pos_y,
                 int icon_size);

void DrawFolder(NodeFolder& folder, TFilter& mFilter, EditorWindow& ed,
                std::optional<std::function<void()>>& activeModal,
                kpi::IObject*& mActive, std::string& mActiveClassId);

} // namespace riistudio::frontend
