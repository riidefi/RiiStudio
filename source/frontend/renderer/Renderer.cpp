#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Renderer.hpp"
#include <core/3d/gl.hpp>    // glPolygonMode
#include <core/util/gui.hpp> // ImGui::BeginMenuBar
#include <frontend/root.hpp> // RootWindow

namespace riistudio::frontend {

Renderer::Renderer(lib3d::IDrawable* root) : mRoot(root) {}
Renderer::~Renderer() {}

void Renderer::render(u32 width, u32 height, bool& showCursor) {
  // The root may set a flag to signal it is undergoing a mutation and is unsafe
  // to be drawn.
  if (mRoot->poisoned)
    return;

  // After the mutation occurs, the root will signal us to reinitialize our
  // cache of it.
  if (mRoot->reinit) {
    mRoot->reinit = false;
    mRoot->prepare(mSceneState, *dynamic_cast<kpi::INode*>(mRoot));
    mSceneState.buildVertexBuffers();
  }

  drawMenuBar();

#ifdef RII_NATIVE_GL_WIREFRAME
  setGlWireframe(wireframe);
#endif

  if (!rend)
    return;

  if (ImGui::IsWindowFocused()) {
    const auto input_state = buildInputState();
    mCameraController.move(1.0f / ImGui::GetIO().Framerate, combo_choice_cam,
                           input_state);
    if (input_state.clickView) {
      RootWindow::spInstance->hideMouse();
    } else {
      RootWindow::spInstance->showMouse();
    }
  } else {
    RootWindow::spInstance->showMouse();
  }

  updateCameraController(mSceneState.computeBounds());

  glm::mat4 projMtx, viewMtx;
  mCameraController.mCamera.calcMatrices(width, height, projMtx, viewMtx);

  mSceneState.buildUniformBuffers(projMtx, viewMtx);

  clearGlScreen();
  mSceneState.draw();
}

void Renderer::drawMenuBar() {
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("Camera")) {
      mCameraController.drawOptions();

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

    ImGui::Combo("##Controls", (int*)&combo_choice_cam,
                 "WASD // FPS\0WASD // Plane\0");

#ifdef RII_NATIVE_GL_WIREFRAME
    ImGui::Checkbox("Wireframe", &wireframe);
#endif
    ImGui::EndMenuBar();
  }
}

void Renderer::updateCameraController(const lib3d::AABB& bound) {
  const f32 dist = glm::distance(bound.min, bound.max);
  mCameraController.mSpeedFactor = dist == 0.0f ? 50.0f : dist / 1000.0f;

  if (mCameraController.mCamera.getPosition() == glm::vec3{0.0f}) {
    const auto min = bound.min;
    const auto max = bound.max;

    mCameraController.mCamera.setPosition((min + max) / 2.0f);

    if (min != max) {
      const auto dist = glm::distance(min, max);
      const auto range = 100000.0f;
      const auto expansion = 5.0f; // Let's assume the user wants to be at least
                                   // 5x away from the model.
      mCameraController.mCamera.setClipPlanes(dist / range * expansion,
                                              dist * expansion);
    }
  }
}

void Renderer::setGlWireframe(bool wireframe) const {
#ifdef RII_NATIVE_GL_WIREFRAME
  if (wireframe)
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  else
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
}

void Renderer::clearGlScreen() const {
  glDepthMask(GL_TRUE);
  const auto bg = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
  glClearColor(bg.x, bg.y, bg.z, bg.w);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

} // namespace riistudio::frontend
