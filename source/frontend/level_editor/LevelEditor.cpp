#include "LevelEditor.hpp"
#include "CppSupport.hpp"
#include "IO.hpp"
#include "KclUtil.hpp"
#include "ObjUtil.hpp"
#include "Transform.hpp"
#include "ViewCube.hpp"
#include <bit>
#include <core/3d/gl.hpp>
#include <core/common.h>
#include <core/util/gui.hpp>
#include <frontend/root.hpp>
#include <frontend/widgets/Manipulator.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <imcxx/Widgets.hpp> // imcxx::Combo
#include <librii/gl/Compiler.hpp>
#include <librii/glhelper/Primitives.hpp>
#include <librii/glhelper/ShaderProgram.hpp>
#include <librii/glhelper/Util.hpp>
#include <librii/glhelper/VBOBuilder.hpp>
#include <librii/math/srt3.hpp>
#include <rsl/Rna.hpp>

namespace riistudio::lvl {

struct VersionReport {
  std::string course_kcl_version;
};

VersionReport MakeVersionReport(const librii::kcol::KCollisionData* kcl) {
  return {
      .course_kcl_version = kcl != nullptr ? kcl->version : "",
  };
}

void DrawRenderOptions(RenderOptions& opt) {
  ImGui::Checkbox("Visual Model", &opt.show_brres);
  ImGui::Checkbox("Collision Model", &opt.show_kcl);
  ImGui::Checkbox("Map Model", &opt.show_map);

  std::string desc = "(" + std::to_string(opt.attr_mask.num_enabled()) +
                     ") Attributes Selected";

  if (ImGui::BeginCombo("Flags", desc.c_str())) {
    for (int i = 0; i < 32; ++i) {
      if (!opt.target_attr_all.is_enabled(i))
        continue;

      const auto& info = GetKCLType(i);
      ImGui::CheckboxFlags(info.name.c_str(), &opt.attr_mask.value,
                           opt.attr_mask.get_flag(i));
    }

    ImGui::EndCombo();
  }

  opt.xlu_mode = imcxx::Combo("Translucency", opt.xlu_mode, "Fast\0Fancy\0");
  ImGui::SliderFloat("Collision Alpha", &opt.kcl_alpha, 0.0f, 1.0f);
}
void LevelEditorWindow::openFile(std::span<const u8> buf, std::string path) {
  // Read .szs
  {
    std::string errc;
    auto root_arc = ReadArchive(buf, errc);
    if (!root_arc.has_value()) {
      mErrDisp = errc;
      return;
    }

    mLevel.root_archive = std::move(*root_arc);
    mLevel.og_path = path;
  }

  setName("Level Editor: " + path);

  // Read course_model.brres
  {
    auto course_model_brres = FindFileWithOverloads(
        mLevel.root_archive, {"course_d_model.brres", "course_model.brres"});
    if (course_model_brres.has_value()) {
      mCourseModel = std::make_unique<RenderableBRRES>(ReadBRRES(
          course_model_brres->file_data, course_model_brres->resolved_path));
    }
  }

  // Read vrcorn_model.brres
  {
    auto vrcorn_model_brres = FindFileWithOverloads(
        mLevel.root_archive, {"vrcorn_d_model.brres", "vrcorn_model.brres"});
    if (vrcorn_model_brres.has_value()) {
      mVrcornModel = std::make_unique<RenderableBRRES>(ReadBRRES(
          vrcorn_model_brres->file_data, vrcorn_model_brres->resolved_path));
    }
  }

  // Read map_model.brres
  {
    auto map_model =
        FindFileWithOverloads(mLevel.root_archive, {"map_model.brres"});
    if (map_model.has_value()) {
      mMapModel = std::make_unique<RenderableBRRES>(
          ReadBRRES(map_model->file_data, map_model->resolved_path));
    }
  }

  // Read course.kcl
  {
    auto course_kcl =
        FindFileWithOverloads(mLevel.root_archive, {"course.kcl"});
    if (course_kcl.has_value()) {
      mCourseKcl = ReadKCL(course_kcl->file_data, course_kcl->resolved_path);
    }
  }

  // Init course.kcl
  if (mCourseKcl) {
    mTriangleRenderer.init(*mCourseKcl);
    disp_opts.init(*mCourseKcl);
  }

  // Read course.kmp
  {
    auto course_kmp =
        FindFileWithOverloads(mLevel.root_archive, {"course.kmp"});
    if (course_kmp.has_value()) {
      mKmp = ReadKMP(course_kmp->file_data, course_kmp->resolved_path);
    }
  }

  // Init course.kmp
  if (mKmp) {
    mKmpHistory.init(*mKmp);

    // Place the camera near the starting point, if it exists
    auto& cam = mRenderSettings.mCameraController.mCamera;
    if (!mKmp->mStartPoints.empty()) {
      auto& start = mKmp->mStartPoints[0];
      cam.mEye = start.position + glm::vec3(0.0f, 10'000.0f, 0.0f);
    }
  }

  // Default camera values
  {
    auto& cam = mRenderSettings.mCameraController.mCamera;

    cam.mClipMin = 200.0f;
    cam.mClipMax = 1000000.0f;
    mRenderSettings.mCameraController.mSpeed = 15'000.0f;
  }
}

void LevelEditorWindow::saveFile(std::string path) {
  // Update archive cache
  if (mKmp != nullptr)
    mLevel.root_archive.files["course.kmp"] = WriteKMP(*mKmp);

  // Flush archive cache
  auto szs_buf = WriteArchive(mLevel.root_archive);

  plate::Platform::writeFile(szs_buf, path);
}

static std::optional<std::pair<std::string, std::vector<u8>>>
GatherNodes(Archive& arc) {
  std::optional<std::pair<std::string, std::vector<u8>>> clicked;
  for (auto& f : arc.folders) {
    if (ImGui::TreeNode((f.first + "/").c_str())) {
      GatherNodes(*f.second.get());
      ImGui::TreePop();
    }
  }
  for (auto& f : arc.files) {
    if (ImGui::Selectable(f.first.c_str())) {
      clicked = f;
    }
  }

  return clicked;
}

static void SetupColumn(const char* name, u32 flags = 0,
                        float init_width_or_weight = -1.0f) {
  ImGui::TableSetupColumn(name, flags | ImGuiTableColumnFlags_WidthFixed,
                          init_width_or_weight > 0 ? init_width_or_weight
                                                   : ImGui::GetFontSize() * 6);
}

struct IDTable {
  // Returns: has_child
  bool addRow(std::string name, size_t size, bool& selected, bool leaf) {
    ImGui::TableNextRow();
    auto d = util::RAIICustomSelectable(selected);
    ImGui::TableNextColumn();
    const bool has_child =
        ImGui::TreeNodeEx(name.c_str(), leaf ? ImGuiTreeNodeFlags_Leaf : 0);
    if (has_child && leaf) {
      ImGui::TreePop();
    }
    if (util::IsNodeSwitchedTo()) {

      selected = true;
    }
    ImGui::TableNextColumn();
    ImGui::Text("%i", static_cast<int>(size));
    return has_child;
  }
  bool autoRow(std::string name, size_t sz, LevelEditorWindow::Page page,
               bool leaf) {
    bool selected = mpWin->mPage == page;
    bool has_child = addRow(name, sz, selected, leaf);
    if (selected) {
      mpWin->mPage = page;
    }
    return has_child;
  }
  bool autoRow(std::string name, const auto& points,
               LevelEditorWindow::Page page, bool leaf) {
    return autoRow(name, points.size(), page, leaf);
  }
  bool autoRowSub(std::string name, LevelEditorWindow::Page page, bool leaf,
                  size_t subindex) {
    bool selected = mpWin->mPage == page && mpWin->mSubPageID == subindex;
    bool has_child = addRow(name, 0, selected, leaf);
    if (selected) {
      mpWin->mPage = page;
      mpWin->mSubPageID = subindex;
    }
    return has_child;
  }
  void endRow() { ImGui::TreePop(); }

  bool Begin() {
    ImGuiTableFlags flags = ImGuiTableFlags_BordersV |
                            ImGuiTableFlags_BordersOuterH |
                            ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg;
    const bool b = ImGui::BeginTable("##3ways", 2, flags);
    if (b) {
      SetupColumn("Name", ImGuiTableColumnFlags_NoHide);
      SetupColumn("ID");
      ImGui::TableHeadersRow();
    }
    return b;
  }
  void End() { ImGui::EndTable(); }

  LevelEditorWindow* mpWin;
};
void LevelEditorWindow::draw_() {
  if (mErrDisp.size()) {
    util::PushErrorSyle();
    ImGui::Text("ERROR: %s", mErrDisp.c_str());
    util::PopErrorStyle();
    return;
  }

  if (ImGui::BeginMenuBar()) {
    if (ImGui::Button("Save")) {
      saveFile("umm.szs");
    }

    ImGui::EndMenuBar();
  }
  if (Begin(".szs", nullptr, 0, this)) {
    auto clicked = GatherNodes(mLevel.root_archive);
    if (clicked.has_value()) {
      auto* pParent = frontend::RootWindow::spInstance;
      if (pParent) {
        pParent->dropDirect(clicked->second, clicked->first);
      }
    }
  }
  ImGui::End();

  if (Begin("View", nullptr, ImGuiWindowFlags_MenuBar, this)) {
    auto bounds = ImGui::GetWindowSize();
    if (mViewport.begin(static_cast<u32>(bounds.x),
                        static_cast<u32>(bounds.y))) {
      drawScene(bounds.x, bounds.y);

      mViewport.end();
    }
  }
  ImGui::End();

  if (mKmp == nullptr)
    return;

  IDTable table;
  table.mpWin = this;

  if (Begin("Tree", nullptr, ImGuiWindowFlags_MenuBar, this)) {

    if (table.Begin()) {
      table.autoRow("Start Points", mKmp->mStartPoints, Page::StartPoints,
                    true);

      if (table.autoRow("Enemy Paths", mKmp->mEnemyPaths, Page::EnemyPaths,
                        false)) {
        for (int i = 0; i < mKmp->mEnemyPaths.size(); ++i) {
          table.autoRowSub("Enemy Path #" + std::to_string(i),
                           Page::EnemyPaths_Sub, true, i);
        }
        table.endRow();
      }
      if (table.autoRow("Item Paths", mKmp->mItemPaths, Page::ItemPaths,
                        false)) {
        for (int i = 0; i < mKmp->mItemPaths.size(); ++i) {
          table.autoRowSub("Item Path #" + std::to_string(i),
                           Page::ItemPaths_Sub, true, i);
        }
        table.endRow();
      }
      if (table.autoRow("Check Paths", mKmp->mCheckPaths, Page::CheckPaths,
                        false)) {
        for (int i = 0; i < mKmp->mCheckPaths.size(); ++i) {
          table.autoRowSub("Check Path #" + std::to_string(i),
                           Page::CheckPaths_Sub, true, i);
        }
        table.endRow();
      }

      table.autoRow("Objects", mKmp->mGeoObjs, Page::Objects, true);
      table.autoRow("Paths", mKmp->mPaths, Page::Paths, true);
      table.autoRow("Areas", mKmp->mAreas, Page::Areas, true);
      table.autoRow("Cameras", mKmp->mCameras, Page::Cameras, true);
      table.autoRow("Respawns", mKmp->mRespawnPoints, Page::Respawns, true);
      table.autoRow("Cannons", mKmp->mCannonPoints, Page::Cannons, true);
      table.autoRow("Mission Points", mKmp->mMissionPoints, Page::MissionPoints,
                    true);
      table.autoRow("Stages", mKmp->mStages, Page::Stages, true);

      table.End();
    }
  }
  ImGui::End();
}

ImGuiID LevelEditorWindow::buildDock(ImGuiID root_id) {
  ImGuiID next = root_id;
  ImGuiID dock_right_id =
      ImGui::DockBuilderSplitNode(next, ImGuiDir_Right, 0.4f, nullptr, &next);
  ImGuiID dock_left_id =
      ImGui::DockBuilderSplitNode(next, ImGuiDir_Left, 0.2f, nullptr, &next);
  ImGuiID dock_right_down_id = ImGui::DockBuilderSplitNode(
      dock_right_id, ImGuiDir_Down, 0.7f, nullptr, &dock_right_id);

  ImGui::DockBuilderDockWindow(idIfyChild("Tree").c_str(), dock_right_id);
  ImGui::DockBuilderDockWindow(idIfyChild("Properties").c_str(),
                               dock_right_down_id);
  ImGui::DockBuilderDockWindow(idIfyChild(".szs").c_str(), dock_left_id);
  ImGui::DockBuilderDockWindow(idIfyChild("View").c_str(), next);
  return next;
}

void PushCube(riistudio::lib3d::SceneState& state, glm::mat4 modelMtx,
              glm::mat4 viewMtx, glm::mat4 projMtx) {

  auto& cube = state.getBuffers().opaque.nodes.emplace_back();
  cube.mega_state = {.cullMode = (u32)-1 /* GL_BACK */,
                     .depthWrite = GL_TRUE,
                     .depthCompare = GL_LEQUAL,
                     .frontFace = GL_CW,

                     .blendMode = GL_FUNC_ADD,
                     .blendSrcFactor = GL_ONE,
                     .blendDstFactor = GL_ZERO};
  cube.shader_id = librii::glhelper::GetCubeShader();
  cube.vao_id = librii::glhelper::GetCubeVAO();
  cube.texture_objects.resize(0);

  cube.primitive_type = librii::gfx::PrimitiveType::Triangles;
  cube.vertex_count = librii::glhelper::GetCubeNumVerts();
  cube.vertex_data_type = librii::gfx::DataType::U32;
  cube.indices = 0; // Offset in VAO

  cube.bound = {};

  glm::mat4 mvp = projMtx * viewMtx * modelMtx;
  librii::gl::UniformSceneParams params{.projection = mvp,
                                        .Misc0 = {69.0f, 0.0f, 0.0f, 0.0f}};

  enum {
    // So UBOBuilder requires the same UBO layout for every use of the same
    // binding point. It also requires there to be 4x binding point 0, 4x
    // binding point 1, etc.. except it doesn't break if we have extra on
    // binding point 0. This is a mess
    UB_SCENEPARAMS_FOR_CUBE_ID = 0
  };

  auto& uniforms =
      cube.uniform_data.emplace_back(librii::gfx::SceneNode::UniformData{
          .binding_point = UB_SCENEPARAMS_FOR_CUBE_ID,
          .raw_data = {sizeof(params)}});

  uniforms.raw_data.resize(sizeof(params));
  memcpy(uniforms.raw_data.data(), &params, sizeof(params));

  glUniformBlockBinding(
      cube.shader_id, glGetUniformBlockIndex(cube.shader_id, "ub_SceneParams"),
      UB_SCENEPARAMS_FOR_CUBE_ID);

  int query_min = 1024;
  glGetActiveUniformBlockiv(cube.shader_id, UB_SCENEPARAMS_FOR_CUBE_ID,
                            GL_UNIFORM_BLOCK_DATA_SIZE, &query_min);
  // This conflicts with the previous min?

  cube.uniform_mins.push_back(librii::gfx::SceneNode::UniformMin{
      .binding_point = UB_SCENEPARAMS_FOR_CUBE_ID,
      .min_size = static_cast<u32>(query_min)});
}

void LevelEditorWindow::drawScene(u32 width, u32 height) {
  mRenderSettings.drawMenuBar(false, false);

  // if (ImGui::BeginMenuBar()) {
  DrawRenderOptions(disp_opts);
  auto report = MakeVersionReport(mCourseKcl.get());
  ImGui::Text("KCL: %s", report.course_kcl_version.c_str());
  //   ImGui::EndMenuBar();
  // }

  if (!mRenderSettings.rend)
    return;

  const auto time_step = mDeltaTimer.tick();
  if (mMouseHider.begin_interaction(ImGui::IsWindowFocused())) {
    const auto input_state = frontend::buildInputState();
    mRenderSettings.mCameraController.move(time_step, input_state);

    mMouseHider.end_interaction(input_state.clickView);
  }

  // Only configure once we have stuff
  if (mSceneState.getBuffers().opaque.nodes.size()) {
    frontend::ConfigureCameraControllerByBounds(
        mRenderSettings.mCameraController, mSceneState.computeBounds(), 1.0f);
    auto& cam = mRenderSettings.mCameraController.mCamera;
    cam.mClipMin = 200.0f;
    cam.mClipMax = 1000000.0f;
  }

  glm::mat4 projMtx, viewMtx;
  mRenderSettings.mCameraController.mCamera.calcMatrices(width, height, projMtx,
                                                         viewMtx);

  mSceneState.invalidate();

  if (disp_opts.show_brres) {
    if (mVrcornModel) {
      mVrcornModel->addNodesToBuffer(mSceneState, viewMtx, projMtx);
    }
    if (mCourseModel) {
      mCourseModel->addNodesToBuffer(mSceneState, viewMtx, projMtx);
    }
  }

  if (mKmp != nullptr) {
    int i = 0;
    switch (mPage) {
    case Page::StartPoints:
      for (auto& pt : mKmp->mStartPoints) {
        glm::mat4 modelMtx = MatrixOfPoint(pt.position, pt.rotation, 1);
        PushCube(mSceneState, modelMtx, viewMtx, projMtx);

        SelectedPath path{.vector_addr = &mKmp->mStartPoints,
                          .index = (size_t)i++};
        if (isSelected(path.vector_addr, path.index)) {
          if (mSelectedObjectTransformEdit.owned_by == path) {
            if (mSelectedObjectTransformEdit.dirty) {
              auto [pos, rot, range] =
                  PointOfMatrix(mSelectedObjectTransformEdit.matrix);

              pt.position = pos;
              pt.rotation = rot;

              mSelectedObjectTransformEdit.dirty = false;
            }
          } else {
            mSelectedObjectTransformEdit = {
                .matrix = modelMtx, .owned_by = path, .dirty = false};
          }
        }
      }
      break;
    case Page::Objects:
      for (auto& pt : mKmp->mGeoObjs) {
        glm::mat4 modelMtx = MatrixOfPoint(pt.position, pt.rotation, 1);
        PushCube(mSceneState, modelMtx, viewMtx, projMtx);
      }
      break;
    case Page::Areas:
      for (auto& pt : mKmp->mAreas) {
        glm::mat4 modelMtx =
            MatrixOfPoint(pt.getModel().mPosition, pt.getModel().mRotation, 1);
        PushCube(mSceneState, modelMtx, viewMtx, projMtx);
      }
      break;
    case Page::Cameras:
      for (auto& pt : mKmp->mCameras) {
        glm::mat4 modelMtx = MatrixOfPoint(pt.mPosition, pt.mRotation, 1);
        PushCube(mSceneState, modelMtx, viewMtx, projMtx);
      }
      break;
    case Page::Respawns:
      for (auto& pt : mKmp->mRespawnPoints) {
        glm::mat4 modelMtx = MatrixOfPoint(pt.position, pt.rotation, pt.range);
        PushCube(mSceneState, modelMtx, viewMtx, projMtx);

        SelectedPath path{.vector_addr = &mKmp->mRespawnPoints,
                          .index = (size_t)i++};
        if (isSelected(path.vector_addr, path.index)) {
          if (mSelectedObjectTransformEdit.owned_by == path) {
            if (mSelectedObjectTransformEdit.dirty) {
              auto [pos, rot, range] =
                  PointOfMatrix(mSelectedObjectTransformEdit.matrix);

              pt.position = pos;
              pt.rotation = rot;

              mSelectedObjectTransformEdit.dirty = false;
            }
          } else {
            mSelectedObjectTransformEdit = {
                .matrix = modelMtx, .owned_by = path, .dirty = false};
          }
        }
      }
      break;
    case Page::Cannons:
      for (auto& pt : mKmp->mCannonPoints) {
        glm::mat4 modelMtx =
            MatrixOfPoint(pt.getPosition(), pt.getRotation(), 1);
        PushCube(mSceneState, modelMtx, viewMtx, projMtx);
      }
      break;
    case Page::MissionPoints:
      for (auto& pt : mKmp->mMissionPoints) {
        glm::mat4 modelMtx = MatrixOfPoint(pt.position, pt.rotation, 1);
        PushCube(mSceneState, modelMtx, viewMtx, projMtx);
      }
      break;
    }
  }

  if (disp_opts.show_kcl && mCourseKcl) {
    // Z sort
    if (disp_opts.xlu_mode == XluMode::Fancy) {
      mTriangleRenderer.sortTriangles(viewMtx);
    }

    mTriangleRenderer.draw(mSceneState, glm::mat4(1.0f), viewMtx, projMtx,
                           disp_opts.attr_mask.value, disp_opts.kcl_alpha);
  }

  if (disp_opts.show_map && mMapModel) {
    auto& opa = mSceneState.getBuffers().opaque.nodes;
    auto& xlu = mSceneState.getBuffers().translucent.nodes;
    // TODO: This assumes the BRRES adds on to the end
    const size_t begin_opa = opa.size();
    const size_t begin_xlu = xlu.size();

    ImGui::InputFloat("Minimap ScaleY", &mini_scale_y);

    mMapModel->addNodesToBuffer(
        mSceneState,
        viewMtx * glm::scale(glm::mat4(1.0f), glm::vec3(1, mini_scale_y, 1)),
        projMtx);

    // Transfer to XLU pass
    xlu.insert(xlu.begin() + begin_xlu, opa.begin() + begin_opa, opa.end());
    opa.resize(begin_opa);

    std::vector<librii::gfx::SceneNode*> map_nodes;
    for (size_t i = begin_xlu; i < xlu.size(); ++i)
      map_nodes.push_back(&xlu[i]);

    for (auto* node : map_nodes) {
      node->mega_state.fill = librii::gfx::PolygonMode::Line;
      node->mega_state.depthCompare = GL_ALWAYS;
      node->mega_state.depthWrite = GL_TRUE;
      node->mega_state.cullMode = -1;
    }
  }

  mSceneState.buildUniformBuffers();

  librii::glhelper::ClearGlScreen();
  mSceneState.draw();

  const bool view_updated =
      DrawViewCube(mViewport.mLastResolution.width,
                   mViewport.mLastResolution.height, viewMtx);
  auto& cam = mRenderSettings.mCameraController;
  if (view_updated)
    frontend::SetCameraControllerToMatrix(cam, viewMtx);

  static Manipulator manip;

  if (!mSelectedObjectTransformEdit.dirty) {
    manip.drawUi(mSelectedObjectTransformEdit.matrix);
    auto backup = mSelectedObjectTransformEdit.matrix;
    manip.manipulate(mSelectedObjectTransformEdit.matrix, viewMtx, projMtx);
    mSelectedObjectTransformEdit.dirty |=
        mSelectedObjectTransformEdit.matrix != backup;
  }

  if (mKmp != nullptr) {
    mKmpHistory.update(*mKmp);
  }
}

} // namespace riistudio::lvl
