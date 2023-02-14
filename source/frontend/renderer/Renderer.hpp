#pragma once

#include "MouseHider.hpp"
#include <LibBadUIFramework/Node2.hpp>
#include <core/3d/i3dmodel.hpp>
#include <core/common.h>
#include <frontend/renderer/CameraController.hpp>
#include <frontend/widgets/DeltaTime.hpp>
#include <glm/mat4x4.hpp>
#include <librii/gfx/SceneState.hpp>

namespace riistudio::frontend {

struct SceneState;

struct RenderSettings {
  // The standard viewport controller
  CameraController mCameraController;

  bool rend = true;
  bool wireframe = false;
  lib3d::RenderType mRenderType{lib3d::RenderType::Preview};

  void drawMenuBar(bool draw_controller = true, bool draw_wireframe = true);
};

class Renderer {
public:
  Renderer(lib3d::IDrawable* root, const lib3d::Scene* data);
  ~Renderer();
  void render(u32 width, u32 height);

  Camera& getCamera() { return mSettings.mCameraController.mCamera; }

private:
  // Scene state
  lib3d::SceneState mSceneState;

  lib3d::IDrawable* mRoot = nullptr;
  const lib3d::Scene* mData = nullptr;
  lib3d::DrawableDispatcher mRootDispatcher;

public:
  RenderSettings mSettings;

  // Overwritten every frame
  glm::mat4 mProjMtx{1}, mViewMtx{1};

private:
  MouseHider mMouseHider;
  lvl::DeltaTimer mDeltaTimer;
};
} // namespace riistudio::frontend
