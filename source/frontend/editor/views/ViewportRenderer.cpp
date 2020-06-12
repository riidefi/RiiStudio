#include "ViewportRenderer.hpp"
#include <frontend/renderer/Renderer.hpp> // Renderer
#include <plate/toolkit/Viewport.hpp>     // plate::tk::Viewport

// TODO: Clean up pointerlock code
#include <core/3d/gl.hpp>
#ifdef __EMSCRIPTEN__
#include <SDL.h>
#include <emscripten.h>
#include <emscripten/html5.h>

extern bool gPointerLock;
#endif

namespace riistudio::frontend {

struct RenderTest : public StudioWindow {
  RenderTest(const kpi::IDocumentNode& host);

private:
  void draw_() override;

  // Components
  plate::tk::Viewport mViewport;
  Renderer mRenderer;

  // Data view
  const kpi::IDocumentNode* model;
  const kpi::IDocumentNode& mHost; // texture
};

RenderTest::RenderTest(const kpi::IDocumentNode& host)
    : StudioWindow("Viewport"), mHost(host) {
  setWindowFlag(ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_MenuBar);

  const auto* models = host.getFolder<lib3d::Model>();

  if (!models || models->empty())
    return;

  model = &models->at<kpi::IDocumentNode>(0);
  mRenderer.prepare(*model, host);
}

void RenderTest::draw_() {
  auto bounds = ImGui::GetWindowSize();

  if (mViewport.begin(static_cast<u32>(bounds.x), static_cast<u32>(bounds.y))) {
    // auto* parent = dynamic_cast<EditorWindow*>(mParent);
    static bool showCursor = false; // TODO
    mRenderer.render(static_cast<u32>(bounds.x), static_cast<u32>(bounds.y),
                     showCursor);

#ifdef RII_BACKEND_GLFW
    if (mpGlfwWindow != nullptr)
      glfwSetInputMode(mpGlfwWindow, GLFW_CURSOR,
                       showCursor ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
#elif defined(RII_BACKEND_SDL) && defined(__EMSCRIPTEN__)
    gPointerLock = !showCursor;
    if (showCursor)
      emscripten_exit_pointerlock();
    else
      emscripten_request_pointerlock(0, EM_TRUE);
      // SDL_SetRelativeMouseMode(!showCursor ? SDL_TRUE : SDL_FALSE);
#endif
    mViewport.end();
  }
}

std::unique_ptr<StudioWindow>
MakeViewportRenderer(const kpi::IDocumentNode& host) {
  return std::make_unique<RenderTest>(host);
}

} // namespace riistudio::frontend
