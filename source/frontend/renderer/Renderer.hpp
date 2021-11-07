#pragma once

#include "MouseHider.hpp"
#include <core/3d/i3dmodel.hpp>
#include <core/3d/renderer/SceneState.hpp>
#include <core/common.h>
#include <core/kpi/Node2.hpp>
#include <frontend/renderer/CameraController.hpp>
#include <glm/mat4x4.hpp>
#include <memory>

namespace riistudio::frontend {

struct SceneState;

struct RenderSettings {
  // The standard viewport controller
  CameraController mCameraController;

  bool rend = true;
  bool wireframe = false;

  void drawMenuBar(bool draw_controller=true, bool draw_wireframe=true);
};

class Renderer {
public:
  Renderer(lib3d::IDrawable* root);
  ~Renderer();
  void render(u32 width, u32 height);

  Camera& getCamera() { return mSettings.mCameraController.mCamera; }

private:
  // Scene state
  lib3d::SceneState mSceneState;

  lib3d::IDrawable* mRoot = nullptr;
  lib3d::DrawableDispatcher mRootDispatcher;

  RenderSettings mSettings;
  MouseHider mMouseHider;
};
} // namespace riistudio::frontend
