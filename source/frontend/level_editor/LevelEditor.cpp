#include "LevelEditor.hpp"
#include <core/common.h>
#include <core/util/gui.hpp>
#include <core/util/oishii.hpp>
#include <frontend/root.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <librii/glhelper/Util.hpp>
#include <librii/kmp/io/KMP.hpp>
#include <librii/math/srt3.hpp>
#include <librii/szs/SZS.hpp>
#include <librii/u8/U8.hpp>
#include <math.h>
#include <optional>
#include <plugins/g3d/G3dIo.hpp>
#include <vendor/ImGuizmo.h>

#include <core/3d/gl.hpp>
#include <librii/gl/Compiler.hpp>
#include <librii/glhelper/ShaderProgram.hpp>
#include <librii/glhelper/VBOBuilder.hpp>

#include <librii/glhelper/Primitives.hpp>

namespace riistudio::lvl {

static std::optional<Archive> ReadArchive(std::span<const u8> buf) {
  auto expanded = librii::szs::getExpandedSize(buf);
  if (expanded == 0) {
    DebugReport("Failed to grab expanded size\n");
    return std::nullopt;
  }

  std::vector<u8> decoded(expanded);
  auto err = librii::szs::decode(decoded, buf);
  if (err) {
    DebugReport("Failed to decode SZS\n");
    return std::nullopt;
  }

  if (decoded.size() < 4 || decoded[0] != 0x55 || decoded[1] != 0xaa ||
      decoded[2] != 0x38 || decoded[3] != 0x2d) {
    DebugReport("Not a valid archive\n");
    return std::nullopt;
  }

  librii::U8::U8Archive arc;
  if (!librii::U8::LoadU8Archive(arc, decoded)) {
    DebugReport("Failed to read archive\n");
    return std::nullopt;
  }

  Archive n_arc;

  struct Pair {
    Archive* folder;
    u32 sibling_next;
  };
  std::vector<Pair> n_path;

  n_path.push_back(
      Pair{.folder = &n_arc, .sibling_next = arc.nodes[0].folder.sibling_next});
  for (int i = 1; i < arc.nodes.size(); ++i) {
    auto& node = arc.nodes[i];

    if (node.is_folder) {
      auto tmp = std::make_unique<Archive>();
      n_path.push_back(
          Pair{.folder = tmp.get(), .sibling_next = node.folder.sibling_next});
      auto& parent = n_path[n_path.size() - 2];
      parent.folder->folders.emplace(node.name, std::move(tmp));
    } else {
      const u32 start_pos = node.file.offset;
      const u32 end_pos = node.file.offset + node.file.size;
      assert(node.file.offset + node.file.size <= arc.file_data.size());
      std::vector<u8> vec(arc.file_data.data() + start_pos,
                          arc.file_data.data() + end_pos);
      n_path.back().folder->files.emplace(node.name, std::move(vec));
    }

    while (!n_path.empty() && i + 1 == n_path.back().sibling_next)
      n_path.resize(n_path.size() - 1);
  }
  assert(n_path.empty());

  // Eliminate the period
  if (n_arc.folders.begin()->first == ".") {
    return *n_arc.folders["."];
  }

  return n_arc;
}

struct Reader {
  oishii::DataProvider mData;
  oishii::BinaryReader mReader;

  Reader(std::string path, const std::vector<u8>& data)
      : mData(OishiiReadFile(path, data.data(), data.size())),
        mReader(mData.slice()) {}
};

struct SimpleTransaction {
  SimpleTransaction() {
    trans.callback = [](...) {};
  }

  bool success() const {
    return trans.state == kpi::TransactionState::Complete;
  }

  kpi::LightIOTransaction trans;
};

std::unique_ptr<g3d::Collection> ReadBRRES(const std::vector<u8>& buf,
                                           std::string path) {
  auto result = std::make_unique<g3d::Collection>();

  SimpleTransaction trans;
  Reader reader(path, buf);
  g3d::ReadBRRES(*result, reader.mReader, trans.trans);

  if (!trans.success())
    return nullptr;

  return result;
}

std::unique_ptr<librii::kmp::CourseMap> ReadKMP(const std::vector<u8>& buf,
                                                std::string path) {
  auto result = std::make_unique<librii::kmp::CourseMap>();

  Reader reader(path, buf);
  librii::kmp::readKMP(*result, reader.mData.slice());

  return result;
}

void LevelEditorWindow::openFile(std::span<const u8> buf, std::string path) {
  auto root_arc = ReadArchive(buf);
  if (!root_arc.has_value())
    return;

  mLevel.root_archive = std::move(*root_arc);
  mLevel.og_path = path;

  auto course_model_brres = FindFile(mLevel.root_archive, "course_model.brres");
  if (course_model_brres.has_value()) {
    mCourseModel = ReadBRRES(*course_model_brres, "course_model.brres");
  }

  auto vrcorn_model_brres = FindFile(mLevel.root_archive, "vrcorn_model.brres");
  if (course_model_brres.has_value()) {
    mVrcornModel = ReadBRRES(*vrcorn_model_brres, "vrcorn_model.brres");
  }

  auto course_kmp = FindFile(mLevel.root_archive, "course.kmp");
  if (course_kmp.has_value()) {
    mKmp = ReadKMP(*course_kmp, "course.kmp");

    mKmpHistory.push_back(*mKmp);
  }

  auto& cam = mRenderSettings.mCameraController.mCamera;
  cam.mClipMin = 200.0f;
  cam.mClipMax = 1000000.0f;
  mRenderSettings.mCameraController.mSpeed = 15'000.0f;

  if (mKmp && !mKmp->mStartPoints.empty()) {
    auto& start = mKmp->mStartPoints[0];
    cam.mEye = start.position + glm::vec3(0.0f, 10'000.0f, 0.0f);
  }
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

void LevelEditorWindow::draw_() {
  if (ImGui::Begin("Hi")) {
    auto clicked = GatherNodes(mLevel.root_archive);
    if (clicked.has_value()) {
      auto* pParent = frontend::RootWindow::spInstance;
      if (pParent) {
        pParent->dropDirect(clicked->second, clicked->first);
      }
    }
  }
  ImGui::End();

  if (ImGui::Begin("View", nullptr, ImGuiWindowFlags_MenuBar)) {
    auto bounds = ImGui::GetWindowSize();
    if (mViewport.begin(static_cast<u32>(bounds.x),
                        static_cast<u32>(bounds.y))) {
      drawScene(bounds.x, bounds.y);

      mViewport.end();
    }
  }
  ImGui::End();
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

  cube.glBeginMode = GL_TRIANGLES;
  cube.vertex_count = librii::glhelper::GetCubeNumVerts();
  cube.glVertexDataType = GL_UNSIGNED_INT;
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

struct Manipulator {
  bool dgui = true;
  ImGuizmo::OPERATION mCurrentGizmoOperation = ImGuizmo::ROTATE;
  ImGuizmo::MODE mCurrentGizmoMode = ImGuizmo::WORLD;
  bool useSnap = false;
  float st[3];
  float sr[3];
  float ss[3];
  float* snap = st;

  void drawUi(glm::mat4& mx) {
    if (ImGui::IsKeyPressed(65 + 'G' - 'A'))
      mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed(65 + 'R' - 'A'))
      mCurrentGizmoOperation = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed(65 + 'T' - 'A'))
      mCurrentGizmoOperation = ImGuizmo::SCALE;
    if (ImGui::RadioButton("Translate",
                           mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
      mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate",
                           mCurrentGizmoOperation == ImGuizmo::ROTATE))
      mCurrentGizmoOperation = ImGuizmo::ROTATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
      mCurrentGizmoOperation = ImGuizmo::SCALE;
    float matrixTranslation[3], matrixRotation[3], matrixScale[3];
    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(mx), matrixTranslation,
                                          matrixRotation, matrixScale);
    ImGui::InputFloat3("Tr", matrixTranslation, 3);
    ImGui::InputFloat3("Rt", matrixRotation, 3);
    ImGui::InputFloat3("Sc", matrixScale, 3);
    ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation,
                                            matrixScale, glm::value_ptr(mx));

    if (mCurrentGizmoOperation != ImGuizmo::SCALE) {
      if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
        mCurrentGizmoMode = ImGuizmo::LOCAL;
      ImGui::SameLine();
      if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
        mCurrentGizmoMode = ImGuizmo::WORLD;
    }
    if (ImGui::IsKeyPressed(83))
      useSnap = !useSnap;
    ImGui::Checkbox("", &useSnap);
    ImGui::SameLine();
    switch (mCurrentGizmoOperation) {
    case ImGuizmo::TRANSLATE:
      snap = st;
      ImGui::InputFloat3("Snap", st);
      break;
    case ImGuizmo::ROTATE:
      snap = sr;
      ImGui::InputFloat("Angle Snap", sr);
      break;
    case ImGuizmo::SCALE:
      snap = ss;
      ImGui::InputFloat("Scale Snap", ss);
      break;
    default:
      snap = st;
    }
  }

  bool manipulate(glm::mat4& mx, const glm::mat4& viewMtx,
                  const glm::mat4& projMtx) {
    return ImGuizmo::Manipulate(glm::value_ptr(viewMtx),
                                glm::value_ptr(projMtx), mCurrentGizmoOperation,
                                mCurrentGizmoMode, glm::value_ptr(mx), NULL,
                                useSnap ? snap : NULL);
  }

  void drawDebugCube(const glm::mat4& mx, const glm::mat4& viewMtx,
                     const glm::mat4& projMtx) {
    ImGuizmo::DrawCubes(glm::value_ptr(viewMtx), glm::value_ptr(projMtx),
                        glm::value_ptr(mx), 1);
  }
};

void LevelEditorWindow::drawScene(u32 width, u32 height) {
  mRenderSettings.drawMenuBar();

  if (!mRenderSettings.rend)
    return;

  librii::glhelper::SetGlWireframe(mRenderSettings.wireframe);

  if (mMouseHider.begin_interaction(ImGui::IsWindowFocused())) {
    // TODO: This time step is a rolling average so is not quite accurate
    // frame-by-frame.
    const auto time_step = 1.0f / ImGui::GetIO().Framerate;
    const auto input_state = frontend::buildInputState();
    mRenderSettings.mCameraController.move(time_step, input_state);

    mMouseHider.end_interaction(input_state.clickView);
  }

  // Only configure once we have stuff
  if (mSceneState.getBuffers().opaque.nodes.size()) {
    frontend::ConfigureCameraControllerByBounds(
        mRenderSettings.mCameraController, mSceneState.computeBounds());
    auto& cam = mRenderSettings.mCameraController.mCamera;
    cam.mClipMin = 200.0f;
    cam.mClipMax = 1000000.0f;
  }

  glm::mat4 projMtx, viewMtx;
  mRenderSettings.mCameraController.mCamera.calcMatrices(width, height, projMtx,
                                                         viewMtx);

  mSceneState.invalidate();

  {
    if (mVrcornModel) {
      mVrcornModel->prepare(mSceneState, *mVrcornModel, viewMtx, projMtx);
    }
    if (mCourseModel) {
      mCourseModel->prepare(mSceneState, *mCourseModel, viewMtx, projMtx);
    }
  }

  if (mKmp != nullptr) {
    for (auto& pt : mKmp->mRespawnPoints) {
      glm::mat4 modelMtx = librii::math::calcXform(
          {.scale =
               glm::vec3(std::max(std::min(pt.range * 50.0f, 1000.0f), 700.0f)),
           .rotation = pt.rotation,
           .translation = pt.position});
      PushCube(mSceneState, modelMtx, viewMtx, projMtx);
    }
  }
  mSceneState.buildUniformBuffers();

  librii::glhelper::ClearGlScreen();
  mSceneState.draw();

  auto pos = ImGui::GetCursorScreenPos();
  // auto win = ImGui::GetWindowPos();
  auto avail = ImGui::GetContentRegionAvail();

  // int shifted_x = mViewport.mLastWidth - avail.x;
  int shifted_y = mViewport.mLastHeight - avail.y;

  // TODO: LastWidth is moved to the right, not left -- bug?
  ImGuizmo::SetRect(pos.x, pos.y - shifted_y, mViewport.mLastWidth,
                    mViewport.mLastHeight);

  auto tVm = viewMtx;
  auto max = ImGui::GetContentRegionMaxAbs();
  ImGuizmo::ViewManipulate(
      glm::value_ptr(viewMtx), 20.0f, {max.x - 200.0f, max.y - 200.0f},
      {200.0f, 200.0f},
      ImGui::GetColorU32(ImGui::GetStyle().Colors[ImGuiCol_WindowBg]));

  auto& cam = mRenderSettings.mCameraController;
  auto cartesian = glm::vec3(glm::vec4(0.0f, 0.0f, -1.0f, 0.0f) * viewMtx);
  {
    riistudio::util::ConditionalHighlight g(true);
    ImGui::Text("Cartesian: %f, %f, %f", cartesian.x, cartesian.y, cartesian.z);
  }
  if (tVm != viewMtx) {

    auto d = glm::distance(cartesian, glm::vec3(0.0f, 0.0f, 0.0f));

    cam.mHorizontalAngle = atan2f(cartesian.x, cartesian.z);
    cam.mVerticalAngle = asinf(cartesian.y / d);
  }

  static Manipulator manip;

  if (mKmp) {

    int q = 0;
    for (auto& pt : mKmp->mRespawnPoints) {
      glm::mat4 mx = librii::math::calcXform(
          {.scale =
               glm::vec3(std::max(std::min(pt.range * 50.0f, 1000.0f), 700.0f)),
           .rotation = pt.rotation,
           .translation = pt.position});
      if (q++ == 0) {

        manip.drawUi(mx);
        if (manip.manipulate(mx, viewMtx, projMtx)) {

          glm::vec3 matrixTranslation, matrixRotation, matrixScale;
          ImGuizmo::DecomposeMatrixToComponents(
              glm::value_ptr(mx), glm::value_ptr(matrixTranslation),
              glm::value_ptr(matrixRotation), glm::value_ptr(matrixScale));

          pt.position = matrixTranslation;
          pt.rotation = matrixRotation;

          assert(pt == pt);
        }
      }
    }
  }

  if (!commit_posted && *mKmp != mKmpHistory[history_cursor]) {
    commit_posted = true;
  }

  if (commit_posted && !ImGui::IsAnyMouseDown()) {
    CommitHistory(history_cursor, mKmpHistory);
    mKmpHistory.push_back(*mKmp);
    assert(mKmpHistory.back() == *mKmp);
    commit_posted = false;
  }

  // TODO: Only affect active window
  if (ImGui::GetIO().KeyCtrl) {
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z))) {
      UndoHistory(history_cursor, mKmpHistory);
      *mKmp = mKmpHistory[history_cursor];
    } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Y))) {
      RedoHistory(history_cursor, mKmpHistory);
      *mKmp = mKmpHistory[history_cursor];
    }
  }
}

} // namespace riistudio::lvl
