#pragma once

#include <glm/mat4x4.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <librii/math/aabb.hpp>
#include <string>
#include <vendor/fa5/IconsFontAwesome5.h>

#include <imgui_markdown.h>

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

} // namespace riistudio::util
