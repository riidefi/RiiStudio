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

      ImGui::SliderFloat("Camera Speed", &speed, 1.0f, 10000.f);
      ImGui::SliderFloat("FOV", &fov, 1.0f, 180.0f);

      ImGui::InputFloat("Near Plane", &cmin);
      ImGui::InputFloat("Far Plane", &cmax);

      ImGui::DragFloat("X", &eye.x, .01f, -10, 30);
      ImGui::DragFloat("Y", &eye.y, .01f, -10, 30);
      ImGui::DragFloat("Z", &eye.z, .01f, -10, 30);

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

  // horizontal angle : toward -Z
  float horizontalAngle = 3.14f;
  // vertical angle : 0, look at the horizon
  float verticalAngle = 0.0f;

  // TODO: Enable for plane mode
  // glm::vec3 up = glm::cross(right, direction);
  glm::vec3 up = {0, 1, 0};

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
    float deltaTime = 1.0f / ImGui::GetIO().Framerate;

    static float xpos = 0;
    static float ypos = 0;

    if (!showCursor) {
      auto pos = ImGui::GetMousePos();

      xpos = pos.x;
      ypos = pos.y;
    }
    horizontalAngle += mouseSpeed * deltaTime * float(width - xpos);
    verticalAngle += mouseSpeed * deltaTime * float(height - ypos);

    // Direction : Spherical coordinates to Cartesian coordinates conversion
    direction =
        glm::vec3(cos(verticalAngle) * sin(horizontalAngle), sin(verticalAngle),
                  cos(verticalAngle) * cos(horizontalAngle));
    glm::vec3 mvmt_dir{direction.x, combo_choice_cam == 0 ? 0.0f : direction.y,
                       direction.z};
    if (combo_choice_cam == 0)
      mvmt_dir = glm::normalize(mvmt_dir);
    glm::vec3 right = glm::vec3(sin(horizontalAngle - 3.14f / 2.0f), 0,
                                cos(horizontalAngle - 3.14f / 2.0f));

#ifdef RII_BACKEND_GLFW
    if (ImGui::IsKeyDown('W'))
#else
    const Uint8* keys = SDL_GetKeyboardState(NULL);

    if (keys[SDL_SCANCODE_W])
#endif
      eye += mvmt_dir * deltaTime * speed;
#ifdef RII_BACKEND_GLFW
    if (ImGui::IsKeyDown('S'))
#else
    if (keys[SDL_SCANCODE_S])
#endif
      eye -= mvmt_dir * deltaTime * speed;
#ifdef RII_BACKEND_GLFW
    if (ImGui::IsKeyDown('A'))
#else
    if (keys[SDL_SCANCODE_A])
#endif
      eye -= right * deltaTime * speed;
#ifdef RII_BACKEND_GLFW
    if (ImGui::IsKeyDown('D'))
#else
    if (keys[SDL_SCANCODE_D])
#endif
      eye += right * deltaTime * speed;
#ifdef RII_BACKEND_GLFW
    if ((ImGui::IsKeyDown(' ') && combo_choice_cam == 0) ||
        ImGui::IsKeyDown('E'))
#else
    if ((keys[SDL_SCANCODE_SPACE] && combo_choice_cam == 0) ||
        keys[SDL_SCANCODE_E])
#endif
      eye += up * deltaTime * speed;
#ifdef RII_BACKEND_GLFW
    if ((ImGui::IsKeyDown(340) && combo_choice_cam == 0) ||
        ImGui::IsKeyDown('Q')) // GLFW_KEY_LEFT_SHIFT
#else
    if ((keys[SDL_SCANCODE_LSHIFT] && combo_choice_cam == 0) ||
        keys[SDL_SCANCODE_Q])
#endif
      eye -= up * deltaTime * speed;
  } else // if (inCtrl)
  {
    inCtrl = false;
    showCursor = true;
  }

  if (!rend)
    return;

#ifdef _WIN32
  if (wireframe)
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  else
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif

  glm::mat4 projMtx = glm::perspective(
      glm::radians(fov), static_cast<f32>(width) / static_cast<f32>(height),
      cmin, cmax);
  glm::mat4 viewMtx = glm::lookAt(eye, eye + direction, up);

  riistudio::core::AABB bound;

  mState->build(projMtx, viewMtx, bound);

  const f32 dist = glm::distance(bound.m_minBounds, bound.m_maxBounds);
  if (speed == 0.0f)
    speed = dist / 10.0f;
  if (speed == 0.0f)
    speed = 6000.0;
  // cmin = std::min(std::min(bound.m_minBounds.x, bound.m_minBounds.y),
  // bound.m_minBounds.z) * 100; cmax = std::max(std::max(bound.m_maxBounds.x,
  // bound.m_maxBounds.y), bound.m_maxBounds.z) * 100;
  if (eye == glm::vec3{0.0f}) {
    const auto min = bound.m_minBounds;
    const auto max = bound.m_maxBounds;

    eye.x = (min.x + max.x) / 2.0f;
    eye.y = (min.y + max.y) / 2.0f;
    eye.z = (min.z + max.z) / 2.0f;
  }

  mState->draw();
}

} // namespace riistudio::frontend
