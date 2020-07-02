#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <string>
#include <vendor/fa5/IconsFontAwesome5.h>

namespace riistudio::util {

struct ConditionalActive {
  ConditionalActive(bool pred, bool flag = true) : bPred(pred), bFlag(flag) {
    if (!bPred) {
      if (bFlag)
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
  }
  ~ConditionalActive() {
    if (!bPred) {
      if (bFlag)
        ImGui::PopItemFlag();
      ImGui::PopStyleVar();
    }
  }
  bool bPred = false, bFlag = true;
};
struct IDScope {
  template <typename T> IDScope(T id) { ImGui::PushID(id); }
  ~IDScope() { ImGui::PopID(); }
};

} // namespace riistudio::util
