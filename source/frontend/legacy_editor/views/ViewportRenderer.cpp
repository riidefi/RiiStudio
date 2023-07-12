#include "ViewportRenderer.hpp"

// TODO: Clean up pointerlock code
#include <core/3d/gl.hpp>
#ifdef __EMSCRIPTEN__
#include <SDL.h>
#include <emscripten.h>
#include <emscripten/html5.h>

extern bool gPointerLock;
#endif

#include <frontend/level_editor/ViewCube.hpp>

namespace riistudio::frontend {

RenderTest::RenderTest(const libcube::Scene& host)
    : StudioWindow("Viewport"), mRenderer(&host) {
  setWindowFlag(ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_MenuBar);
  setClosable(false);
}

void RenderTest::precache() {
  rsl::info("Precaching render state");
  mRenderer.precache();
}

void RenderTest::draw_() {
  auto bounds = ImGui::GetWindowSize();

  if (mViewport.begin(static_cast<u32>(bounds.x), static_cast<u32>(bounds.y))) {
    // auto* parent = dynamic_cast<EditorWindow*>(mParent);

    mRenderer.render(static_cast<u32>(bounds.x), static_cast<u32>(bounds.y));

    drawViewCube();

    mViewport.end();
  }
}

void RenderTest::drawViewCube() {
  const bool view_updated =
      lvl::DrawViewCube(mViewport.mLastResolution.width,
                        mViewport.mLastResolution.height, mRenderer.mViewMtx);
  auto& cam = mRenderer.mSettings.mCameraController;
  if (view_updated)
    frontend::SetCameraControllerToMatrix(cam, mRenderer.mViewMtx);
}

} // namespace riistudio::frontend
