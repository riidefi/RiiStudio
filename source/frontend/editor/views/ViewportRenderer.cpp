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
  RenderTest(const kpi::INode& host);

private:
  void draw_() override;

  // Components
  plate::tk::Viewport mViewport;
  Renderer mRenderer;
};

RenderTest::RenderTest(const kpi::INode& host)
    : StudioWindow("Viewport"), mRenderer(dynamic_cast<lib3d::IDrawable*>(
                                    const_cast<kpi::INode*>(&host))) {
  setWindowFlag(ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_MenuBar);
  setClosable(false);
  mRenderer.prepare(host);
}

void RenderTest::draw_() {
  auto bounds = ImGui::GetWindowSize();

  if (mViewport.begin(static_cast<u32>(bounds.x), static_cast<u32>(bounds.y))) {
    // auto* parent = dynamic_cast<EditorWindow*>(mParent);
    static bool showCursor = false; // TODO
    mRenderer.render(static_cast<u32>(bounds.x), static_cast<u32>(bounds.y),
                     showCursor);

    mViewport.end();
  }
}

std::unique_ptr<StudioWindow> MakeViewportRenderer(const kpi::INode& host) {
  return std::make_unique<RenderTest>(host);
}

} // namespace riistudio::frontend
