#include "Renderer.hpp"
#include "SceneState.hpp"    // SceneState
#include <core/3d/gl.hpp>    // glDebugMessageCallback
#include <core/util/gui.hpp> // ImGui::BeginMenuBar

#ifndef _WIN32
#include <SDL.h>
#include <SDL_opengles2.h>
#endif

// Hack
#include <plugins/j3d/Shape.hpp>
#include <vendor/glm/matrix.hpp>

#undef min

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
  mState->build(projMtx, viewMtx, bound);

  const f32 dist = glm::distance(bound.min, bound.max);
  if (mCamera.getSpeed() == 0.0f)
    mCamera.setSpeed(dist / 10.0f);
  if (mCamera.getSpeed() == 0.0f)
    mCamera.setSpeed(6000.0);

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

  mState->draw();
}

} // namespace riistudio::frontend
