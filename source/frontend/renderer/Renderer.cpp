#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Renderer.hpp"
#include <core/3d/gl.hpp>    // glPolygonMode
#include <core/util/gui.hpp> // ImGui::BeginMenuBar

#ifndef _WIN32
#include <SDL.h>
#include <SDL_opengles2.h>
#endif

// Hack
#include <plugins/j3d/Shape.hpp>
#include <vendor/glm/matrix.hpp>

namespace riistudio::frontend {

Renderer::Renderer(lib3d::IDrawable* root) : mRoot(root) {}
Renderer::~Renderer() {}

void Renderer::render(u32 width, u32 height, bool& showCursor) {
  if (mRoot->poisoned)
    return;
  if (mRoot->reinit) {
    mRoot->reinit = false;
    prepare(*dynamic_cast<kpi::INode*>(mRoot));
  }

  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("Camera")) {
      ImGui::SliderFloat("Mouse Speed", &mouseSpeed, 0.0f, .2f);

      mCamera.drawOptions();

      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Rendering")) {
      ImGui::Checkbox("Render", &rend);
#ifdef RII_NATIVE_GL_WIREFRAME
      ImGui::Checkbox("Wireframe", &wireframe);
#endif
      ImGui::EndMenu();
    }
    // static int combo_choice = 0;
    // ImGui::Combo("Shading", &combo_choice, "Emulated
    // Shaders\0Normals\0TODO\0");

    ImGui::Combo("##Controls", &combo_choice_cam,
                 "WASD // FPS\0WASD // Plane\0");

#ifdef RII_NATIVE_GL_WIREFRAME
    ImGui::Checkbox("Wireframe", &wireframe);
#endif
    ImGui::EndMenuBar();
  }

#ifdef RII_NATIVE_GL_WIREFRAME
  if (wireframe)
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  else
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif

  const bool mouseDown = ImGui::IsAnyMouseDown();
  // Disabled this feature, not useful.
  const bool ctrlPress = false;
  // ImGui::IsKeyPressed(341); // GLFW_KEY_LEFT_CONTROL

  if (ImGui::IsWindowFocused()) {
    if (ctrlPress) {
      if (inCtrl) {
        inCtrl = false;
        showCursor = true;
      } else {
        inCtrl = true;
        showCursor = false;
      }
    } else if (!inCtrl) {
      showCursor = !mouseDown;
    }
  } else // if (inCtrl)
  {
    inCtrl = false;
    showCursor = true;
  }

  if (!rend)
    return;

  bool key_w = false, key_s = false, key_a = false, key_d = false,
       key_up = false, key_down = false;

  if (ImGui::IsWindowFocused()) {
#ifdef RII_BACKEND_GLFW
    if (ImGui::IsKeyDown('W'))
      key_w = true;
    if (ImGui::IsKeyDown('A'))
      key_a = true;
    if (ImGui::IsKeyDown('S'))
      key_s = true;
    if (ImGui::IsKeyDown('D'))
      key_d = true;
    if ((ImGui::IsKeyDown(' ') && combo_choice_cam == 0) ||
        ImGui::IsKeyDown('E'))
      key_up = true;
    if ((ImGui::IsKeyDown(340) && combo_choice_cam == 0) ||
        ImGui::IsKeyDown('Q')) // GLFW_KEY_LEFT_SHIFT
      key_down = true;
#else
    const Uint8* keys = SDL_GetKeyboardState(NULL);

    if (keys[SDL_SCANCODE_W])
      key_w = true;
    if (keys[SDL_SCANCODE_A])
      key_a = true;
    if (keys[SDL_SCANCODE_S])
      key_s = true;
    if (keys[SDL_SCANCODE_D])
      key_d = true;
    if ((keys[SDL_SCANCODE_SPACE] && combo_choice_cam == 0) ||
        keys[SDL_SCANCODE_E])
      key_up = true;
    if ((keys[SDL_SCANCODE_LSHIFT] && combo_choice_cam == 0) ||
        keys[SDL_SCANCODE_Q])
      key_down = true;
#endif
  }

  glm::mat4 projMtx, viewMtx;
  mCamera.calc(showCursor, mouseSpeed, combo_choice_cam, width, height, projMtx,
               viewMtx, key_w, key_a, key_s, key_d, key_up, key_down);

  riistudio::lib3d::AABB bound;
  mRoot->build(projMtx, viewMtx, bound);

  const f32 dist = glm::distance(bound.min, bound.max);
  mCamera.setSpeedFactor(dist == 0.0f ? 50.0f : dist / 1000.0f);

  if (mCamera.getPosition() == glm::vec3{0.0f}) {
    const auto min = bound.min;
    const auto max = bound.max;

    glm::vec3 eye;
    eye.x = (min.x + max.x) / 2.0f;
    eye.y = (min.y + max.y) / 2.0f;
    eye.z = (min.z + max.z) / 2.0f;
    mCamera.setPosition(eye);

    if (min != max) {
      const auto dist = glm::distance(min, max);
      const auto range = 100000.0f;
      const auto expansion = 5.0f; // Let's assume the user wants to be at least
                                   // 5x away from the model.
      mCamera.setClipPlanes(dist / range * expansion, dist * expansion);
    }
  }

  const auto bg = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
  glClearColor(bg.x, bg.y, bg.z, bg.w);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  mRoot->draw();
}

} // namespace riistudio::frontend
