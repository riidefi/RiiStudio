#pragma once

#include "MouseHider.hpp"
#include <LibBadUIFramework/Node2.hpp>
#include <core/3d/i3dmodel.hpp>
#include <core/common.h>
#include <frontend/renderer/CameraController.hpp>
#include <frontend/widgets/DeltaTime.hpp>
#include <glm/mat4x4.hpp>
#include <librii/gfx/SceneState.hpp>

#include <librii/g3d/gfx/G3dGfx.hpp>

namespace riistudio::frontend {

class SceneImpl {
public:
  Result<void> upload(const libcube::Scene& host);
  Result<void> prepare(librii::gfx::SceneState& state,
                       const libcube::Scene& host,
                       glm::mat4 v_mtx, glm::mat4 p_mtx,
                       lib3d::RenderType type);

private:
  librii::g3d::gfx::G3dSceneRenderData render_data;
  bool uploaded = false;
};

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
  Renderer(const libcube::Scene* data);
  ~Renderer();
  void render(u32 width, u32 height);
  void precache();

  Camera& getCamera() { return mSettings.mCameraController.mCamera; }

private:
  // Scene state
  librii::gfx::SceneState mSceneState;

  SceneImpl mRoot;
  const libcube::Scene* mData = nullptr;

public:
  RenderSettings mSettings;

  // Overwritten every frame
  glm::mat4 mProjMtx{1}, mViewMtx{1};

private:
  MouseHider mMouseHider;
  lvl::DeltaTimer mDeltaTimer;
};
} // namespace riistudio::frontend
