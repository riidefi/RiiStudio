#include "InputState.hpp"
#include <imgui/imgui.h>

#ifdef __EMSCRIPTEN__
#include <SDL.h>
#include <SDL_opengles2.h>
#endif

namespace riistudio::frontend {

InputState buildInputState() {
  bool key_w = false, key_s = false, key_a = false, key_d = false,
       key_up = false, key_down = false, mouse_select = false,
       mouse_view = false;

#ifdef RII_BACKEND_GLFW
  if (ImGui::IsKeyDown('W'))
    key_w = true;
  if (ImGui::IsKeyDown('A'))
    key_a = true;
  if (ImGui::IsKeyDown('S'))
    key_s = true;
  if (ImGui::IsKeyDown('D'))
    key_d = true;
  if (ImGui::IsKeyDown(' ') || ImGui::IsKeyDown('E'))
    key_up = true;
  if (ImGui::IsKeyDown(340) || ImGui::IsKeyDown('Q')) // GLFW_KEY_LEFT_SHIFT
    key_down = true;
#elif defined(__EMSCRIPTEN__)
  const Uint8* keys = SDL_GetKeyboardState(NULL);

  if (keys[SDL_SCANCODE_W])
    key_w = true;
  if (keys[SDL_SCANCODE_A])
    key_a = true;
  if (keys[SDL_SCANCODE_S])
    key_s = true;
  if (keys[SDL_SCANCODE_D])
    key_d = true;
  if (keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_E])
    key_up = true;
  if (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_Q])
    key_down = true;
#endif
  if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
    mouse_select = true;
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
