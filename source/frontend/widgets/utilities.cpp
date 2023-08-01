#include "utilities.hpp"
#include <optional>

namespace riistudio::frontend {

void DrawItemBorder(const ImVec4& color,
                    std::optional<ImRect> padding = std::nullopt) {
  DrawItemBorder(ImGui::ColorConvertFloat4ToU32(color), padding);
}

void DrawItemBorder(ImU32 color, std::optional<ImRect> padding = std::nullopt) {
  // Then, we get the bounding box of the last item
  ImRect bb = {ImGui::GetItemRectMin(), ImGui::GetItemRectMax()};
  if (padding) {
    bb.Min -= padding->Min;
    bb.Max += padding->Max;
  }

  // Then we use the ImGui draw list to draw a custom border
  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  draw_list->AddRect(bb.Min, bb.Max, color); // RGBA
}

} // namespace riistudio::frontend
