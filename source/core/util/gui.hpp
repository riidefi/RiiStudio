#pragma once

#include <glm/mat4x4.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <librii/math/aabb.hpp>
#include <string>
#include <vendor/fa5/IconsFontAwesome5.h>

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
  bool bPred = false, bFlag = true;
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

} // namespace riistudio::util
