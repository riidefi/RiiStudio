#pragma once

#include <LibBadUIFramework/Node2.hpp> // kpi::IDocumentNode
#include <core/3d/i3dmodel.hpp>
#include <frontend/legacy_editor/StudioWindow.hpp> // StudioWindow
#include <frontend/renderer/Renderer.hpp>          // Renderer
#include <plate/toolkit/Viewport.hpp>              // plate::tk::Viewport

namespace libcube {
class Scene;
}

namespace riistudio::frontend {

class RenderTest : public StudioWindow {
public:
  RenderTest(const libcube::Scene& host);
  void draw_() override;

  // TODO: Figure out how to do this concurrently
  void precache();

private:
  void drawViewCube();

  // Components
  plate::tk::Viewport mViewport;
  Renderer mRenderer;
};

} // namespace riistudio::frontend
