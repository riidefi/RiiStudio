#pragma once

#include <core/common.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <vendor/magic_enum/magic_enum.hpp>

#include <glm/mat4x4.hpp>
#include <imgui_markdown.h>
#include <librii/math/aabb.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

#include <imcxx/ContiguousSelection.hpp>
#include <imcxx/IndentedTreeWidget.hpp>

static inline ImVec4 Clr(u32 x) {
  return ImVec4{
      static_cast<float>(x >> 16) / 255.0f,
      static_cast<float>((x >> 8) & 0xff) / 255.0f,
      static_cast<float>(x & 0xff) / 255.0f,
      1.0f,
  };
}

namespace imcxx {

static inline bool ColoredButton(u32 color, const char* txt) {
  ImGui::PushStyleColor(ImGuiCol_Button, color);
  bool b = ImGui::Button(txt);
  ImGui::PopStyleColor();
  return b;
}
template <typename... T>
static inline bool ColoredButton(u32 color, fmt::format_string<T...> s,
                                 T&&... args) {
  auto buf = fmt::format(s, std::forward<T>(args)...);
  return ColoredButton(color, buf.c_str());
}

inline int Combo(const std::string& label, const int current_item,
                 const char* options) {
  int trans_item = current_item;
  ImGui::Combo(label.c_str(), &trans_item, options);
  return trans_item;
}

template <typename T>
inline T Combo(const std::string& label, T current_item, const char* options) {
  return static_cast<T>(Combo(label, static_cast<int>(current_item), options));
}

template <typename T>
inline T EnumCombo(const std::string& label, T currentItem) {
  static_assert(magic_enum::enum_entries<T>().size() >= 1);
  std::string options;
  auto entries = magic_enum::enum_entries<T>();
  for (auto& [e, name] : entries) {
    options.append(name);
    options.resize(options.size() + 1); // Add null terminator
  }
  // Let the library decide between a linear / binary search / switch / etc
  auto idx = magic_enum::enum_index<T>(currentItem);
  if (!idx.has_value()) {
    ImGui::Text("%s: Invalid enum value %u", label.c_str(),
                static_cast<u32>(currentItem));
    return currentItem;
  }
  auto cur = *idx;
  auto result = Combo(label, cur, options.c_str());
  if (result >= entries.size() || result < 0) {
    return entries[0].first;
  }
  return entries[result].first;
}

inline bool BeginFullscreenWindow(const char* label, bool* open) {
  if (open != nullptr && !*open)
    return false;

  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->Pos);
  ImGui::SetNextWindowSize(viewport->Size);
  ImGui::SetNextWindowViewport(viewport->ID);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
  window_flags |=
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

  const bool result = ImGui::Begin(label, open, window_flags);
  ImGui::PopStyleVar(3);
  return result;
}
inline void EndFullscreenWindow() { ImGui::End(); }

// Used for HorizTabBar and VertTabBar
struct TabBar {
  std::string name = "Pane";

  std::vector<std::string> tabs;
  int active = 0; // index
};

inline void DrawHorizTabBar(TabBar& bar) {
  if (ImGui::BeginTabBar(bar.name.c_str())) {
    for (size_t i = 0; i < bar.tabs.size(); ++i) {
      if (ImGui::BeginTabItem(bar.tabs[i].c_str())) {
        bar.active = i;
        ImGui::EndTabItem();
      }
    }
    ImGui::EndTabBar();
  }
}

inline void DrawVertTabBar(TabBar& bar) {
  for (size_t i = 0; i < bar.tabs.size(); ++i) {
    const bool sel = i == bar.active;
    if (ImGui::Selectable(bar.tabs[i].c_str(), sel)) {
      bar.active = i;
    }
  }
}

struct CheckBoxTabBar {
  std::string name = "Checkboxes Widget";

  std::vector<std::string> tabs;
  std::vector<bool> active; // index
};

inline void DrawCheckBoxTabBar(CheckBoxTabBar& bar) {
  if (bar.active.size() != bar.tabs.size()) {
    bar.active.resize(bar.tabs.size());
    std::fill(bar.active.begin(), bar.active.end(), true);
  }

  ImGui::BeginTable(bar.name.c_str(), bar.tabs.size() / 5 + 1);
  ImGui::TableNextRow();
  for (size_t i = 0; i < bar.tabs.size(); ++i) {
    ImGui::TableSetColumnIndex(i / 5);

    bool tmp = bar.active[i];
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});
    ImGui::Checkbox(bar.tabs[i].c_str(), &tmp);
    ImGui::PopStyleVar();
    bar.active[i] = tmp;
  }
  ImGui::EndTable();
}

} // namespace imcxx

struct Defer {
  Defer(std::function<void()> f) : mF(f) {}
  Defer(Defer&& rhs) : mF(rhs.mF) { rhs.mF = nullptr; }
  ~Defer() {
    if (mF)
      mF();
  }

  std::function<void()> mF;
};

namespace Toolkit {

inline void BoundingVolume(librii::math::AABB* bbox, float* sphere = nullptr) {
  if (bbox != nullptr) {
    ImGui::InputFloat3("Minimum Point", &bbox->min.x);
    ImGui::InputFloat3("Maximum Point", &bbox->max.x);

    ImGui::Text("Distance: %f", glm::distance(bbox->min, bbox->max));
  }
  if (sphere != nullptr) {
    ImGui::InputFloat("Sphere Radius", sphere);
  }
}
inline void Matrix44(const glm::mat4& mtx) {
  for (int i = 0; i < 4; ++i) {
    ImGui::Text("%f %f %f %f", mtx[i][0], mtx[i][1], mtx[i][2], mtx[i][3]);
  }
}
} // namespace Toolkit

namespace riistudio::util {

struct ConditionalEnabled {
  ConditionalEnabled(bool pred) : m_pred(pred) {
    if (m_pred)
      return;
    auto disabled = ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];

    ImGui::PushStyleColor(ImGuiCol_Text, disabled);
  }
  ~ConditionalEnabled() {
    if (m_pred)
      return;
    ImGui::PopStyleColor();
  }
  bool m_pred = true;
};

struct ConditionalActive {
  ConditionalActive(bool pred) { ImGui::BeginDisabled(!pred); }
  ~ConditionalActive() { ImGui::EndDisabled(); }
};
struct ConditionalBold {
  ConditionalBold(bool pred) : bPred(pred) {
    if (!bPred) {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{.6f, .6f, .6f, 1.0f});
    }
  }
  ~ConditionalBold() {
    if (!bPred) {
      ImGui::PopStyleColor();
    }
  }
  bool bPred = false;
};
struct ConditionalHighlight {
  ConditionalHighlight(bool pred) : bPred(pred) {
    if (bPred) {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0f, 1.0f, 0.0f, 1.0f});
    }
  }
  ~ConditionalHighlight() {
    if (bPred) {
      ImGui::PopStyleColor();
    }
  }
  bool bPred = false, bFlag = true;
};
struct IDScope {
  template <typename T> IDScope(T id) { ImGui::PushID(id); }
  ~IDScope() { ImGui::PopID(); }
};
static inline bool IsNodeSwitchedTo() {
  return ImGui::IsItemClicked() || ImGui::IsItemFocused();
}

static inline bool BeginCustomSelectable(bool sel) {
  if (!sel) {
    ImGui::Text("( )");
    ImGui::SameLine();
    return false;
  }

  ImGui::Text("(X)");
  ImGui::SameLine();
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0f, 1.0f, 0.0f, 1.0f});
  return true;
}
static inline void EndCustomSelectable(bool sel) {
  if (!sel)
    return;

  ImGui::PopStyleColor();
}

static inline Defer RAIICustomSelectable(bool sel) {
  bool b = BeginCustomSelectable(sel);
  return Defer{[b] { EndCustomSelectable(b); }};
}

// Seems we don't need to care about the tree_node_ex_result
// static bool IsNodeSwitchedTo(bool tree_node_ex_result) {
//   return tree_node_ex_result && IsNodeSwitchedTo();
// }

// You can make your own Markdown function with your prefered string container
// and markdown config.
static ImGui::MarkdownConfig mdConfig{
    nullptr,
    nullptr,
    reinterpret_cast<const char*>(ICON_FA_LINK),
    {{NULL, true}, {NULL, true}, {NULL, false}}};

inline void Markdown(const std::string& markdown_) {
  // fonts for, respectively, headings H1, H2, H3 and beyond
  ImGui::Markdown(markdown_.c_str(), markdown_.length(), mdConfig);
}

inline void PushErrorSyle() {
  ImGui::SetWindowFontScale(2.0f);
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0f, 0.0f, 0.0f, 1.0f});
}

inline void PopErrorStyle() {
  ImGui::PopStyleColor();
  ImGui::SetWindowFontScale(1.0f);
}

inline ImGuiTableColumn* GetNextColumn() {
  ImGuiContext& g = *ImGui::GetCurrentContext();
  ImGuiTable* table = g.CurrentTable;
  IM_ASSERT(table != NULL &&
            "Need to call TableSetupColumn() after BeginTable()!");
  IM_ASSERT(!table->IsLayoutLocked &&
            "Need to call call TableSetupColumn() before first row!");
  IM_ASSERT(table->DeclColumnsCount >= 0 &&
            table->DeclColumnsCount < table->ColumnsCount &&
            "Called TableSetupColumn() too many times!");

  ImGuiTableColumn* column = &table->Columns[table->DeclColumnsCount];
  IM_ASSERT(column);

  return column;
}

inline void SetNextColumnVisible(bool shown) {
  auto* column = GetNextColumn();

  column->IsVisibleX = shown;
  column->IsVisibleY = shown;
  // column->IsVisibleNextFrame = shown;
}

//! @brief Determines if a context popup should open based on the last drawn
//! ImGui widget.
inline bool ShouldContextOpen(
    ImGuiPopupFlags popup_flags = ImGuiPopupFlags_MouseButtonRight) {
  ImGuiContext& g = *GImGui;
  ImGuiWindow* window = g.CurrentWindow;
  if (window->SkipItems)
    return false;
  int mouse_button = (popup_flags & ImGuiPopupFlags_MouseButtonMask_);
  return ImGui::IsMouseReleased(mouse_button) &&
         ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
}

inline ImVec4 ColorConvertU32ToFloat4BE(u32 in) {
  in = (in >> 24) | (((in >> 16) & 0xff) << 8) | (((in >> 8) & 0xff) << 16) |
       ((in & 0xff) << 24);
  return ImGui::ColorConvertU32ToFloat4(in);
}

inline u32 ColorConvertFloat4ToU32BE(ImVec4 in) {
  u32 clr = ImGui::ColorConvertFloat4ToU32(in);
  return (clr >> 24) | (((clr >> 16) & 0xff) << 8) |
         (((clr >> 8) & 0xff) << 16) | ((clr & 0xff) << 24);
}

} // namespace riistudio::util
