#pragma once

#include <imgui/imgui.h>
#include <string>
#include <vector>
#include <vendor/magic_enum/magic_enum.hpp>

namespace imcxx {

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
