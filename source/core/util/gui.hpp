#pragma once

#include <vendor/imgui/imgui.h>
#include <vendor/imgui/imgui_internal.h>
#include <string>
#include <vendor/fa5/IconsFontAwesome5.h>

namespace riistudio::util {

struct ConditionalActive {
  ConditionalActive(bool pred) : bPred(pred) {
    if (!bPred) {
      ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
  }
  ~ConditionalActive() {
    if (!bPred) {
      ImGui::PopItemFlag();
      ImGui::PopStyleVar();
    }
  }
  bool bPred = false;
};
struct IDScope {
  template <typename T> IDScope(T id) { ImGui::PushID(id); }
  ~IDScope() { ImGui::PopID(); }
};

inline bool ends_with(const std::string& value, const std::string& ending) {
  return ending.size() <= value.size() &&
         std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

} // namespace riistudio::util
