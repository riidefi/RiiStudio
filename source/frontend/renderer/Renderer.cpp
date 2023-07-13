#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Renderer.hpp"
#include <core/3d/gl.hpp>    // glPolygonMode
#include <frontend/root.hpp> // RootWindow
#include <imcxx/Widgets.hpp>
#include <librii/glhelper/Util.hpp> // librii::glhelper::SetGlWireframe

namespace riistudio::frontend {

// Calculate the bounding box of a polygon
librii::math::AABB CalcPolyBound(const lib3d::Polygon& poly,
                                 const lib3d::Bone& bone,
                                 const lib3d::Model& mdl) {
  auto mdl_mtx = lib3d::calcSrtMtxSimple(bone, &mdl);

  librii::math::AABB bound = poly.getBounds();
  auto nmax = mdl_mtx * glm::vec4(bound.max, 1.0f);
  auto nmin = mdl_mtx * glm::vec4(bound.min, 1.0f);

  return {nmin, nmax};
}

Result<void> SceneImpl::upload(const libcube::Scene& host) {
  if (!uploaded) {
    uploaded = true;
    TRY(render_data.init(host));
  }
  return {};
}

Result<void> SceneImpl::prepare(librii::gfx::SceneState& state,
                                const libcube::Scene& host, glm::mat4 v_mtx,
                                glm::mat4 p_mtx, lib3d::RenderType type) {
  TRY(upload(host));
  return librii::g3d::gfx::Any3DSceneAddNodesToBuffer(
      state, host, glm::mat4(1.0f), v_mtx, p_mtx, render_data, type);
}

void RenderSettings::drawMenuBar(bool draw_controller, bool draw_wireframe) {
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("Camera"_j)) {
      mCameraController.drawOptions();

      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Rendering"_j)) {
      ImGui::Checkbox("Render Scene?"_j, &rend);
      if (draw_wireframe && librii::glhelper::IsGlWireframeSupported())
        ImGui::Checkbox("Wireframe Mode"_j, &wireframe);
      ImGui::EndMenu();
    }

    ImGui::SetNextItemWidth(120.0f * ImGui::GetIO().FontGlobalScale);
    mRenderType = imcxx::EnumCombo("##mRenderType", mRenderType);

    ImGui::SetNextItemWidth(120.0f * ImGui::GetIO().FontGlobalScale);
    mCameraController.drawProjectionOption();

    {
      util::ConditionalActive a(false);

      {
        //   util::ConditionalBold g(true);
        ImGui::TextUnformatted("Backend:");
      }
      ImGui::Text("OpenGL %s", librii::glhelper::GetGlVersion());

      {
        //    util::ConditionalBold g(true);
        ImGui::TextUnformatted("Device:");
      }
      ImGui::TextUnformatted(librii::glhelper::GetGpuName());
    }

    ImGui::EndMenuBar();
  }
}

Renderer::Renderer(const libcube::Scene* node) : mData(node) {}
Renderer::~Renderer() {}

void Renderer::precache() {
  assert(mData != nullptr);
  mRoot.upload(*mData);
}

void Renderer::render(u32 width, u32 height) {
  mSettings.drawMenuBar();

  if (!mSettings.rend)
    return;

  librii::glhelper::SetGlWireframe(mSettings.wireframe);

  const auto time_step = mDeltaTimer.tick();
  if (mMouseHider.begin_interaction(ImGui::IsWindowFocused())) {
    const auto input_state = buildInputState();
    mSettings.mCameraController.move(time_step, input_state);

    mMouseHider.end_interaction(input_state.clickView);
  }

  ConfigureCameraControllerByBounds(mSettings.mCameraController,
                                    mSceneState.computeBounds());

  mSettings.mCameraController.calc();
  mSettings.mCameraController.mCamera.calcMatrices(width, height, mProjMtx,
                                                   mViewMtx);

  mSceneState.invalidate();
  assert(mData != nullptr);
  auto ok = mRoot.prepare(mSceneState, *mData, mViewMtx, mProjMtx,
                          mSettings.mRenderType);
  if (!ok.has_value()) {
    ImGui::TextColored(ImGui::GetStyle().Colors[ImGuiCol_NavHighlight],
                       "Renderer error during populate(): %s",
                       ok.error().c_str());
  }
  mSceneState.buildUniformBuffers();

  librii::glhelper::ClearGlScreen();
  mSceneState.draw();
}

} // namespace riistudio::frontend
