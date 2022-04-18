#include "InputState.hpp"
#include <imgui/imgui.h>

namespace riistudio::frontend {

InputState buildInputState() {
  const bool key_w = ImGui::IsKeyDown(ImGuiKey_W);
  const bool key_a = ImGui::IsKeyDown(ImGuiKey_A);
  const bool key_s = ImGui::IsKeyDown(ImGuiKey_S);
  const bool key_d = ImGui::IsKeyDown(ImGuiKey_D);

  const bool key_up =
      ImGui::IsKeyDown(ImGuiKey_Space) || ImGui::IsKeyDown(ImGuiKey_E);
  const bool key_down =
      ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_Q);
  const bool l_alt = ImGui::IsKeyDown(ImGuiKey_LeftAlt);

  // Left + !LAlt   -> Select
  // Left + LAlt    -> Pan/Orbit
  // Right | Middle -> Pan/Orbit
  //
  bool mouse_view = false;
  bool mouse_select = false;
  if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    if (l_alt)
      mouse_view = true;
    else
      mouse_select = true;
  }

  if (ImGui::IsMouseDown(ImGuiMouseButton_Right) ||
      ImGui::IsMouseDown(ImGuiMouseButton_Middle))
    mouse_view = true;

  std::optional<InputState::MouseState> mouse;

  if (ImGui::IsMousePosValid()) {
    mouse = {.position =
                 glm::vec2(ImGui::GetMousePos().x, ImGui::GetMousePos().y),
             .scroll = ImGui::GetIO().MouseWheel};
  }

  return {.forward = key_w,
          .left = key_a,
          .backward = key_s,
          .right = key_d,
          .up = key_up,
          .down = key_down,
          .clickSelect = mouse_select,
          .clickView = mouse_view,
          .mouse = mouse};
}

} // namespace riistudio::frontend
