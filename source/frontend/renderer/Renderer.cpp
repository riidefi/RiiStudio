#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Renderer.hpp"
#include <core/3d/gl.hpp>           // glPolygonMode
#include <core/util/gui.hpp>        // ImGui::BeginMenuBar
#include <frontend/root.hpp>        // RootWindow
#include <librii/glhelper/Util.hpp> // librii::glhelper::SetGlWireframe

namespace riistudio::frontend {

void RenderSettings::drawMenuBar() {
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("Camera"_j)) {
      mCameraController.drawOptions();

      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Rendering"_j)) {
      ImGui::Checkbox("Render Scene?"_j, &rend);
      if (librii::glhelper::IsGlWireframeSupported())
        ImGui::Checkbox("Wireframe Mode"_j, &wireframe);
      ImGui::EndMenu();
    }

    mCameraController.drawControllerTypeOption();

    if (librii::glhelper::IsGlWireframeSupported())
      ImGui::Checkbox("Wireframe", &wireframe);

    ImGui::EndMenuBar();
  }
}

Renderer::Renderer(lib3d::IDrawable* root) : mRoot(root) {
  root->dispatcher = &mRootDispatcher;
}
Renderer::~Renderer() {}

void Renderer::render(u32 width, u32 height) {
  mSettings.drawMenuBar();

  if (!mSettings.rend)
    return;

  if (!mRootDispatcher.beginDraw())
    return;

  librii::glhelper::SetGlWireframe(mSettings.wireframe);

  if (mMouseHider.begin_interaction(ImGui::IsWindowFocused())) {
    // TODO: This time step is a rolling average so is not quite accurate
    // frame-by-frame.
    const auto time_step = 1.0f / ImGui::GetIO().Framerate;
    const auto input_state = buildInputState();
    mSettings.mCameraController.move(time_step, input_state);

    mMouseHider.end_interaction(input_state.clickView);
  }

  ConfigureCameraControllerByBounds(mSettings.mCameraController,
                                    mSceneState.computeBounds());

  glm::mat4 projMtx, viewMtx;
  mSettings.mCameraController.mCamera.calcMatrices(width, height, projMtx,
                                                   viewMtx);

  mSceneState.invalidate();
  mRootDispatcher.populate(*mRoot, mSceneState,
                           *dynamic_cast<kpi::INode*>(mRoot), projMtx, viewMtx);
  mSceneState.buildUniformBuffers();

  librii::glhelper::ClearGlScreen();
  mSceneState.draw();

  mRootDispatcher.endDraw();
}

} // namespace riistudio::frontend
