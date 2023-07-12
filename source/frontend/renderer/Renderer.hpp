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

class DrawableDispatcher;

// Used for the root element
struct IDrawable {
  virtual ~IDrawable() = default;

  //! Prepare a scene based on the resource data.
  virtual Result<void> prepare(lib3d::SceneState& state,
                               const libcube::Scene& root, glm::mat4 v_mtx,
                               glm::mat4 p_mtx, lib3d::RenderType type) = 0;

  DrawableDispatcher& getDispatcher() {
    assert(dispatcher);
    return *dispatcher;
  }

  DrawableDispatcher* dispatcher = nullptr;
};

class DrawableDispatcher {
public:
  void beginEdit() { poisoned = true; }
  void endEdit() {
    poisoned = false;
    reinit = true;
  }

  Result<void> populate(IDrawable& drawable, lib3d::SceneState& state,
                        const libcube::Scene& root, glm::mat4 v_mtx,
                        glm::mat4 p_mtx, lib3d::RenderType type) {
    assert(!poisoned);
    assert(!reinit);
    return drawable.prepare(state, root, v_mtx, p_mtx, type);
  }

  bool beginDraw() {
    if (poisoned)
      return false;

    // We don't have any initialization code yet
    reinit = false;

    return true;
  }

  void endDraw() {}

private:
  bool poisoned = false;
  bool reinit = false;
};

class SceneImpl : public IDrawable {
public:
  SceneImpl() = default;
  virtual ~SceneImpl() = default;

  Result<void> prepare(lib3d::SceneState& state, const libcube::Scene& host,
                       glm::mat4 v_mtx, glm::mat4 p_mtx,
                       lib3d::RenderType type) override;

  void gatherBoneRecursive(lib3d::SceneBuffers& output, u64 boneId,
                           const lib3d::Model& root, const lib3d::Scene& scn,
                           glm::mat4 v_mtx, glm::mat4 p_mtx);

  void gather(lib3d::SceneBuffers& output, const lib3d::Model& root,
              const lib3d::Scene& scene, glm::mat4 v_mtx, glm::mat4 p_mtx);

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
  Renderer(IDrawable* root, const libcube::Scene* data);
  ~Renderer();
  void render(u32 width, u32 height);

  Camera& getCamera() { return mSettings.mCameraController.mCamera; }

private:
  // Scene state
  lib3d::SceneState mSceneState;

  IDrawable* mRoot = nullptr;
  const libcube::Scene* mData = nullptr;
  DrawableDispatcher mRootDispatcher;

public:
  RenderSettings mSettings;

  // Overwritten every frame
  glm::mat4 mProjMtx{1}, mViewMtx{1};

private:
  MouseHider mMouseHider;
  lvl::DeltaTimer mDeltaTimer;
};
} // namespace riistudio::frontend
