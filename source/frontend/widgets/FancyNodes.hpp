#pragma once

#include <imcxx/Widgets.hpp>
#include <span>
#include <string_view>

namespace lib3d {
struct Texture;
}

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

struct FancyObjectResult {
  bool wasOpened = false;
  bool thereWasAClick = false;
  bool focused = false;
  bool contextMenu = false;
};

FancyObjectResult
FancyObject(size_t i, bool curNodeSelected, ImVec4 type_icon_color,
            std::string type_icon, std::string public_name,
            int display_id_relative_to_parent, bool leaf,
            std::span<const lib3d::Texture*> icons_right,
            std::function<void(const lib3d::Texture*, float)> drawIcon) {
  const float icon_size = 24.0f * ImGui::GetIO().FontGlobalScale;

  FancyObjectResult result{};

  auto id = std::format("{}", display_id_relative_to_parent);
  {
    const auto sel_id = id + "##sel";
    ImGui::Selectable(sel_id.c_str(), curNodeSelected, ImGuiSelectableFlags_None,
                      {0, icon_size});
    if (util::ShouldContextOpen()) {
      result.contextMenu = true;
    }

    ImGui::SameLine();
  }

  result.thereWasAClick = ImGui::IsItemClicked();
  result.focused = ImGui::IsItemFocused();

  u32 flags = ImGuiTreeNodeFlags_DefaultOpen;
  if (leaf)
    flags |= ImGuiTreeNodeFlags_Leaf;

  const auto text_size = ImGui::CalcTextSize("T");
  const auto initial_pos_y = ImGui::GetCursorPosY();
  ImGui::SetCursorPosY(initial_pos_y + (icon_size - text_size.y) / 2.0f);

  ImGui::PushStyleColor(ImGuiCol_Text, type_icon_color);
  const auto treenode =
      ImGui::TreeNodeEx(id.c_str(), flags, "%s", type_icon.c_str());
  ImGui::PopStyleColor();
  ImGui::SameLine();
  ImGui::TextUnformatted(public_name.c_str());
  if (treenode) {
    for (auto* pImg : icons_right) {
      ImGui::SameLine();
      ImGui::SetCursorPosY(initial_pos_y);
      drawIcon(pImg, icon_size);
    }
  }
  ImGui::SetCursorPosY(initial_pos_y + icon_size);
  result.wasOpened = treenode;
  return result;
}

} // namespace riistudio::frontend
