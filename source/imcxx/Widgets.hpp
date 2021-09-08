#pragma once

#include <imgui/imgui.h>
#include <string>

namespace imcxx {

inline int Combo(const std::string& label, const int current_item,
                 const char* options) {
  int trans_item = current_item;
  ImGui::Combo(label.c_str(), &trans_item, options);
  return trans_item;
}

template <typename T>
inline T Combo(const std::string& label, const T current_item,
               const char* options) {
  return static_cast<T>(Combo(label, static_cast<int>(current_item), options));
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

} // namespace imcxx
