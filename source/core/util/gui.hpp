#pragma once

#include <vendor/imgui/imgui.h>
#include <vendor/imgui/imgui_internal.h>

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

} // namespace riistudio::util
