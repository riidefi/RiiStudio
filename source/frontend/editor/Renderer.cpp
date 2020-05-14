#include "Renderer.hpp"

#include <core/3d/gl.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <iostream>

#include <array>
#include <imgui/imgui.h>

#include "renderer/ShaderCache.hpp"
#include "renderer/ShaderProgram.hpp"
#include "renderer/UBOBuilder.hpp"

#include <core/3d/aabb.hpp>

#ifndef _WIN32
#include <SDL.h>
#include <SDL_opengles2.h>
#endif

// Hack
#include <plugins/j3d/Shape.hpp>
#include <vendor/glm/matrix.hpp>

#undef min
#include <algorithm>

#include "renderer/SceneState.hpp"
#include "renderer/SceneTree.hpp"

namespace riistudio::frontend {

void Renderer::prepare(const kpi::IDocumentNode& model,
                       const kpi::IDocumentNode& texture, bool buf, bool tex) {
  mState->gather(model, texture, buf, tex);
}

#ifdef _WIN32
static void cb(GLenum source, GLenum type, GLuint id, GLenum severity,
               GLsizei length, const GLchar* message, GLvoid* userParam) {
  printf("%s\n", message);
}
#endif

Renderer::Renderer() {
#ifdef _WIN32
  glDebugMessageCallback(cb, 0);
#endif

  mState = std::make_unique<SceneState>();
}
Renderer::~Renderer() {}

void Renderer::render(u32 width, u32 height, bool& showCursor) {
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("Camera")) {
      ImGui::SliderFloat("Mouse Speed", &mouseSpeed, 0.0f, .2f);

      mCamera.drawOptions();

      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Rendering")) {
      ImGui::Checkbox("Render", &rend);
#ifdef _WIN32
      ImGui::Checkbox("Wireframe", &wireframe);
#endif
      ImGui::EndMenu();
    }
    static int combo_choice = 0;
    ImGui::Combo("Shading", &combo_choice, "Emulated Shaders\0Normals\0TODO");

    ImGui::Combo("Controls", &combo_choice_cam, "WASD // FPS\0WASD // Plane\0");

#ifdef _WIN32
    ImGui::Checkbox("Wireframe", &wireframe);
#endif
    ImGui::EndMenuBar();
  }

#ifdef _WIN32
  if (wireframe)
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  else
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif

  bool mouseDown = ImGui::IsAnyMouseDown();
  bool ctrlPress = ImGui::IsKeyPressed(341); // GLFW_KEY_LEFT_CONTROL

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

#ifdef RII_BACKEND_GLFW
  if (ImGui::IsKeyDown('W'))
#else
  const Uint8* keys = SDL_GetKeyboardState(NULL);

  if (keys[SDL_SCANCODE_W])
#endif
    key_w = true;
#ifdef RII_BACKEND_GLFW
  if (ImGui::IsKeyDown('S'))
#else
  if (keys[SDL_SCANCODE_S])
#endif
    key_s = true;
#ifdef RII_BACKEND_GLFW
  if (ImGui::IsKeyDown('A'))
#else
  if (keys[SDL_SCANCODE_A])
#endif
    key_d = true;
#ifdef RII_BACKEND_GLFW
  if (ImGui::IsKeyDown('D'))
#else
  if (keys[SDL_SCANCODE_D])
#endif
    key_d = true;
#ifdef RII_BACKEND_GLFW
  if ((ImGui::IsKeyDown(' ') && combo_choice_cam == 0) || ImGui::IsKeyDown('E'))
#else
  if ((keys[SDL_SCANCODE_SPACE] && combo_choice_cam == 0) ||
      keys[SDL_SCANCODE_E])
#endif
    key_up = true;
#ifdef RII_BACKEND_GLFW
  if ((ImGui::IsKeyDown(340) && combo_choice_cam == 0) ||
      ImGui::IsKeyDown('Q')) // GLFW_KEY_LEFT_SHIFT
#else
  if ((keys[SDL_SCANCODE_LSHIFT] && combo_choice_cam == 0) ||
      keys[SDL_SCANCODE_Q])
#endif
    key_down = true;

  glm::mat4 projMtx, viewMtx;
  mCamera.calc(showCursor, mouseSpeed, combo_choice_cam, width, height, projMtx,
               viewMtx, key_w, key_a, key_s, key_d, key_up, key_down);

  riistudio::core::AABB bound;
  mState->build(projMtx, viewMtx, bound);

  const f32 dist = glm::distance(bound.m_minBounds, bound.m_maxBounds);
  if (mCamera.getSpeed() == 0.0f)
    mCamera.setSpeed(dist / 10.0f);
  if (mCamera.getSpeed() == 0.0f)
    mCamera.setSpeed(6000.0);
  // cmin = std::min(std::min(bound.m_minBounds.x, bound.m_minBounds.y),
  // bound.m_minBounds.z) * 100; cmax = std::max(std::max(bound.m_maxBounds.x,
  // bound.m_maxBounds.y), bound.m_maxBounds.z) * 100;
  if (mCamera.getPosition() == glm::vec3{0.0f}) {
    const auto min = bound.m_minBounds;
    const auto max = bound.m_maxBounds;

    glm::vec3 eye;
    eye.x = (min.x + max.x) / 2.0f;
    eye.y = (min.y + max.y) / 2.0f;
    eye.z = (min.z + max.z) / 2.0f;
    mCamera.setPosition(eye);
  }

  mState->draw();
}

} // namespace riistudio::frontend
