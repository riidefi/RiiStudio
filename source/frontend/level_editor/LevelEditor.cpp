#include "LevelEditor.hpp"
#include "IO.hpp"
#include "KclUtil.hpp"
#include "ObjUtil.hpp"
#include "Transform.hpp"
#include <bit>
#include <core/3d/gl.hpp>
#include <core/common.h>
#include <core/util/gui.hpp>
#include <core/util/oishii.hpp>
#include <frontend/root.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>  // glm::distance2
#include <imcxx/Widgets.hpp> // imcxx::Combo
#include <librii/gl/Compiler.hpp>
#include <librii/glhelper/Primitives.hpp>
#include <librii/glhelper/ShaderProgram.hpp>
#include <librii/glhelper/Util.hpp>
#include <librii/glhelper/VBOBuilder.hpp>
#include <librii/math/srt3.hpp>
#include <rsl/Rna.hpp>
#include <vendor/ImGuizmo.h>

namespace riistudio::lvl {

void DrawRenderOptions(RenderOptions& opt) {
  ImGui::Checkbox("Visual Model", &opt.show_brres);
  ImGui::Checkbox("Collision Model", &opt.show_kcl);

  std::string desc = "(" + std::to_string(std::popcount(opt.attr_mask)) +
                     ") Attributes Selected";

  if (ImGui::BeginCombo("Flags", desc.c_str())) {
    for (int i = 0; i < 32; ++i) {
      if ((opt.target_attr_all & (1 << i)) == 0)
        continue;

      const auto& info = GetKCLType(i);
      ImGui::CheckboxFlags(info.name.c_str(), &opt.attr_mask, 1 << i);
    }

    ImGui::EndCombo();
  }

  opt.xlu_mode = imcxx::Combo("Translucency", opt.xlu_mode, "Fast\0Fancy\0");
  ImGui::SliderFloat("Collision Alpha", &opt.kcl_alpha, 0.0f, 1.0f);
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

  auto course_kcl = FindFile(mLevel.root_archive, "course.kcl");
  if (course_kcl.has_value()) {
    mCourseKcl = ReadKCL(*course_kcl, "course.kcl");
  }

  if (mCourseKcl) {
    mKclTris.resize(mCourseKcl->prism_data.size());
    for (size_t i = 0; i < mKclTris.size(); ++i) {
      auto& prism = mCourseKcl->prism_data[i];

      mKclTris[i] = {.attr = prism.attribute,
                     .verts = librii::kcol::FromPrism(*mCourseKcl, prism)};
    }

    for (auto& prism : mCourseKcl->prism_data) {
      disp_opts.target_attr_all |= 1 << (prism.attribute & 31);
    }
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

struct Property {
  std::string name;

  rsl::RNA type;
  bool is_mutable = true;

  size_t offset;
  size_t size;
};

#define FLOAT_P(Name, Mem)                                                     \
  Property {                                                                   \
    .name = Name,                                                              \
    .type = rsl::to_rna<decltype(librii::kmp::RespawnPoint::Mem)>,             \
    .is_mutable = true, .offset = offsetof(librii::kmp::RespawnPoint, Mem),    \
    .size = sizeof(librii::kmp::RespawnPoint::Mem)                             \
  }

std::array<Property, 8> respawn_properties = {
    FLOAT_P("Position.x", position.x), FLOAT_P("Position.y", position.y),
    FLOAT_P("Position.z", position.z), FLOAT_P("Rotation.x", rotation.x),
    FLOAT_P("Rotation.y", rotation.y), FLOAT_P("Rotation.z", rotation.z),
    FLOAT_P("Radius", range),          FLOAT_P("ID", id),
};

#undef FLOAT_P

std::string to_json(const librii::kmp::RespawnPoint& r) {
  std::string result = "{\n";
  for (int i = 0; i < respawn_properties.size(); ++i) {
    const auto& p = respawn_properties[i];
    result += "  \"";
    result += p.name;
    result += "\": ";
    result += std::to_string(
        rsl::ReadAny(rsl::BytesOf(r).subspan(p.offset), p.type).value);
    if (i + 1 != respawn_properties.size())
      result += ",\n";
  }

  return result + "\n}";
}

void LevelEditorWindow::DrawRespawnTable() {
  if (selection.size()) {
    auto j = to_json(mKmp->mRespawnPoints[selection.begin()->index]);
    ImGui::Text(j.c_str());
  }
  auto flags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable |
               ImGuiTableFlags_NoSavedSettings;
  if (ImGui::BeginTable("##TBL2", respawn_properties.size() + 2, flags)) {
    ImGui::TableSetupColumn("Selected");
    ImGui::TableSetupColumn("Index");
    for (auto& p : respawn_properties)
      ImGui::TableSetupColumn(p.name.c_str());
    ImGui::TableAutoHeaders();

    for (int i = 0; i < mKmp->mRespawnPoints.size(); ++i) {
      auto& pt = mKmp->mRespawnPoints[i];
      util::IDScope g(i);

      ImGui::TableNextRow();

      bool selected = isSelected(&mKmp->mRespawnPoints, i);
      if (ImGui::Checkbox("##Selected", &selected)) {
        selection.clear();
        setSelected(&mKmp->mRespawnPoints, i, selected);
      }
      ImGui::TableNextCell();

      ImGui::Text("%i", i);
      ImGui::TableNextCell();

      for (auto& p : respawn_properties) {
        auto bytes = rsl::BytesOf(pt).subspan(p.offset);
        auto any = rsl::ReadAny(bytes, p.type);

        ImGui::InputDouble(p.name.c_str(), &any.value);

        rsl::WriteAny(bytes, any);

        ImGui::TableNextCell();
      }
    }
    ImGui::EndTable();
  }
}

void LevelEditorWindow::draw_() {
  if (ImGui::BeginMenuBar()) {
    if (ImGui::Button("Save")) {
      saveFile("umm.szs");
    }

    ImGui::EndMenuBar();
  }
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

  if (ImGui::Begin("Tree", nullptr, ImGuiWindowFlags_MenuBar)) {
    if (ImGui::TreeNodeEx("Start Positions", ImGuiTreeNodeFlags_Leaf)) {
      if (ImGui::IsItemClicked())
        mPage = Page::StartPoints;
      ImGui::TreePop();
    }
    if (ImGui::TreeNode("Enemy Paths")) {
      if (ImGui::IsItemClicked())
        mPage = Page::EnemyPaths;
      ImGui::TreePop();
    }
    if (ImGui::TreeNode("Item Paths")) {
      if (ImGui::IsItemClicked())
        mPage = Page::ItemPaths;
      ImGui::TreePop();
    }
    if (ImGui::TreeNode("Checkpoints")) {
      if (ImGui::IsItemClicked())
        mPage = Page::CheckPaths;
      ImGui::TreePop();
    }
    if (ImGui::TreeNode("Objects")) {
      if (ImGui::IsItemClicked())
        mPage = Page::Objects;
      ImGui::TreePop();
    }
    if (ImGui::TreeNode("Paths")) {
      if (ImGui::IsItemClicked())
        mPage = Page::Paths;
      ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Areas", ImGuiTreeNodeFlags_Leaf)) {
      if (ImGui::IsItemClicked())
        mPage = Page::Areas;
      ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Cameras", ImGuiTreeNodeFlags_Leaf)) {
      if (ImGui::IsItemClicked())
        mPage = Page::Cameras;
      ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Respawns", ImGuiTreeNodeFlags_Leaf)) {
      if (ImGui::IsItemClicked())
        mPage = Page::Respawns;
      ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Cannons", ImGuiTreeNodeFlags_Leaf)) {
      if (ImGui::IsItemClicked())
        mPage = Page::Cannons;
      ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Mission Points", ImGuiTreeNodeFlags_Leaf)) {
      if (ImGui::IsItemClicked())
        mPage = Page::MissionPoints;
      ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Stages", ImGuiTreeNodeFlags_Leaf)) {
      if (ImGui::IsItemClicked())
        mPage = Page::Stages;
      ImGui::TreePop();
    }
  }
  ImGui::End();

  if (ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_MenuBar)) {
    switch (mPage) {
    case Page::StartPoints:
      break;
    case Page::Respawns:
      DrawRespawnTable();
      break;
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

const char* gTriShader = R"(
#version 400

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in uint attr_id;

out vec4 fragmentColor;
out float _is_wire;

layout(std140) uniform ub_SceneParams {
    mat4x4 u_Projection;
    vec4 u_Misc0;
};

void main() {	
  gl_Position = u_Projection * vec4(position, 1);
  fragmentColor = color;

  // Wire
  bool is_wire = uint(u_Misc0[2]) == 1;
  if (is_wire) {
    // fragmentColor = vec4(1, 1, 1, 1);
    fragmentColor = vec4(mix(normalize(vec3(color)) * sqrt(3), vec3(1, 1, 1), .5), 1);
  }
  _is_wire = is_wire ? 1.0 : 0.0;


  // Precision hack
  uint low_bits = uint(u_Misc0[0]) & 0xFFFF;
  uint high_bits = uint(u_Misc0[1]) << 16;
  uint attr_mask = low_bits | high_bits;

  fragmentColor.w = u_Misc0[3];

  if ((attr_id & attr_mask) == 0) {
    fragmentColor.w = 0.0;
  }
}
)";
const char* gTriShaderFrag = R"(
#version 330 core

in vec4 fragmentColor;
in float _is_wire;

out vec4 color;

void main() {
  if (fragmentColor.w == 0.0) discard;

  color = fragmentColor;

  // bool is_wire = color == vec4(1, 1, 1, 1);
  bool is_wire = _is_wire != 0.0f;

  if (!gl_FrontFacing && !is_wire)
    color *= vec4(.5, .5, .5, 1.0);

  gl_FragDepth = gl_FragCoord.z * (is_wire ? .99991 : .99999);
}
)";

void PushTriangles(riistudio::lib3d::SceneState& state, glm::mat4 modelMtx,
                   glm::mat4 viewMtx, glm::mat4 projMtx, u32 tri_vao,
                   u32 tri_vao_count, u32 attr_mask, float alpha) {
  static const librii::glhelper::ShaderProgram tri_shader(gTriShader,
                                                          gTriShaderFrag);

  auto& cube = state.getBuffers().translucent.nodes.emplace_back();
  static float poly_ofs = 0.0f;
  // ImGui::InputFloat("Poly_ofs", &poly_ofs);
  static float poly_fact = 0.0f;
  // ImGui::InputFloat("Poly_fact", &poly_fact);
  cube.mega_state = {.cullMode = (u32)-1 /* GL_BACK */,
                     .depthWrite = GL_FALSE,
                     .depthCompare = GL_LEQUAL /* GL_LEQUAL */,
                     .frontFace = GL_CCW,

                     .blendMode = GL_FUNC_ADD,
                     .blendSrcFactor = GL_SRC_ALPHA,
                     .blendDstFactor = GL_ONE_MINUS_SRC_ALPHA,

                     // Prevents z-fighting, always win
                     .poly_offset_factor = poly_fact,
                     .poly_offset_units = poly_ofs};
  cube.shader_id = tri_shader.getId();
  cube.vao_id = tri_vao;
  cube.texture_objects.resize(0);

  cube.glBeginMode = GL_TRIANGLES;
  cube.vertex_count = tri_vao_count;
  cube.glVertexDataType = GL_UNSIGNED_INT;
  cube.indices = 0; // Offset in VAO

  cube.bound = {};

  glm::mat4 mvp = projMtx * viewMtx * modelMtx;
  librii::gl::UniformSceneParams params{
      .projection = mvp,
      .Misc0 = {/* attr_mask_lo */ attr_mask & 0xFFFF,
                /* attr_mask_hi */ attr_mask >> 16,
                /* is_wire */ 0.0f, /* alpha */ alpha}};

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

  auto& cube_wire = state.getBuffers().translucent.nodes.emplace_back(cube);

  cube_wire.mega_state.fill = librii::gfx::PolygonMode::Line;

  reinterpret_cast<librii::gl::UniformSceneParams*>(
      cube_wire.uniform_data[0].raw_data.data())
      ->Misc0[2] = 1.0f;
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

  // if (ImGui::BeginMenuBar()) {
  DrawRenderOptions(disp_opts);
  //   ImGui::EndMenuBar();
  // }

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
      mVrcornModel->prepare(mSceneState, *mVrcornModel, viewMtx, projMtx);
    }
    if (mCourseModel) {
      mCourseModel->prepare(mSceneState, *mCourseModel, viewMtx, projMtx);
    }
  }

  if (mKmp != nullptr) {
    for (auto& pt : mKmp->mRespawnPoints) {
      glm::mat4 modelMtx = MatrixOfPoint(pt.position, pt.rotation, pt.range);
      PushCube(mSceneState, modelMtx, viewMtx, projMtx);
    }
  }

  if (tri_vbo == nullptr) {
    tri_vbo = std::make_unique<librii::glhelper::VBOBuilder>();

    tri_vbo->mPropogating[0].descriptor =
        librii::glhelper::VAOEntry{.binding_point = 0,
                                   .name = "position",
                                   .format = GL_FLOAT,
                                   .size = sizeof(glm::vec3)};
    tri_vbo->mPropogating[1].descriptor =
        librii::glhelper::VAOEntry{.binding_point = 1,
                                   .name = "color",
                                   .format = GL_FLOAT,
                                   .size = sizeof(glm::vec4)};
    tri_vbo->mPropogating[2].descriptor =
        librii::glhelper::VAOEntry{.binding_point = 2,
                                   .name = "attr_id",
                                   .format = GL_UNSIGNED_INT,
                                   .size = sizeof(u32)};

    for (int i = 0; i < 3 * mKclTris.size(); ++i) {
      tri_vbo->mIndices.push_back(static_cast<u32>(tri_vbo->mIndices.size()));
      tri_vbo->pushData(/*binding_point=*/0, mKclTris[i / 3].verts[i % 3]);
      tri_vbo->pushData(
          /*binding_point=*/1,
          glm::vec4(GetKCLColor(mKclTris[i / 3].attr), 1.0f));
      tri_vbo->pushData(/*binding_point=*/2,
                        static_cast<u32>(1 << (mKclTris[i / 3].attr & 31)));
    }
    tri_vbo->build();
  }

  if (disp_opts.show_kcl && tri_vbo != nullptr) {
    // Z sort
    if (disp_opts.xlu_mode == XluMode::Fancy) {
      struct index_tri {
        u32 points[3];

        const glm::vec3& get_point(librii::glhelper::VBOBuilder& tri_vbo,
                                   u32 i) const {
          return reinterpret_cast<const glm::vec3&>(
              tri_vbo.mPropogating[0].data[points[i] * sizeof(glm::vec3)]);
        }

        float get_sphere_dist(librii::glhelper::VBOBuilder& tri_vbo, u32 i,
                              const glm::vec3& eye) const {
          // Don't sqrt
          return glm::distance2(get_point(tri_vbo, i), eye);
        }

        float get_min_sphere_dist(librii::glhelper::VBOBuilder& tri_vbo,
                                  const glm::vec3& view) const {
          return std::min({get_sphere_dist(tri_vbo, 0, view),
                           get_sphere_dist(tri_vbo, 1, view),
                           get_sphere_dist(tri_vbo, 2, view)});
        }

        float get_plane_dist(librii::glhelper::VBOBuilder& tri_vbo, u32 i,
                             const glm::mat4& view) const {
          // We assume all points are closer, or all are further away
          return (view * glm::vec4(get_point(tri_vbo, i), 1.0f)).z;
        }

        float get_min_plane_dist(librii::glhelper::VBOBuilder& tri_vbo,
                                 const glm::mat4& view) const {
          return std::max({get_plane_dist(tri_vbo, 0, view),
                           get_plane_dist(tri_vbo, 1, view),
                           get_plane_dist(tri_vbo, 2, view)});
        }
      };

      index_tri* begin = reinterpret_cast<index_tri*>(tri_vbo->mIndices.data());
      index_tri* end = begin + tri_vbo->mIndices.size() / 3;

      z_dist.resize(end - begin);

      // Assumes no vertex merging, uses first vertex as a key
      for (int i = 0; i < end - begin; ++i) {
        auto first_vtx = begin[i].points[0];
        z_dist[first_vtx / 3] = begin[i].get_min_plane_dist(*tri_vbo, viewMtx);
      }

      std::sort(begin, end, [&](const index_tri& lhs, const index_tri& rhs) {
        return z_dist[lhs.points[0] / 3] > z_dist[rhs.points[0] / 3];
      });

      std::reverse(begin, end);

      tri_vbo->uploadIndexBuffer();
    }

    PushTriangles(mSceneState, glm::mat4(1.0f), viewMtx, projMtx,
                  tri_vbo->getGlId(), 3 * mKclTris.size(), disp_opts.attr_mask,
                  disp_opts.kcl_alpha);
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

  const u32 viewcube_bg = 0;
  // ImGui::GetColorU32(ImGui::GetStyle().Colors[ImGuiCol_WindowBg])

  ImGuizmo::ViewManipulate(glm::value_ptr(viewMtx), 20.0f,
                           {max.x - 200.0f, max.y - 200.0f}, {200.0f, 200.0f},
                           viewcube_bg);

  auto& cam = mRenderSettings.mCameraController;
  if (tVm != viewMtx)
    frontend::SetCameraControllerToMatrix(cam, viewMtx);

  static Manipulator manip;

  if (mKmp) {
    for (int i = 0; i < mKmp->mRespawnPoints.size(); ++i) {
      if (!isSelected(&mKmp->mRespawnPoints, i))
        continue;

      auto& pt = mKmp->mRespawnPoints[i];
      glm::mat4 mx = MatrixOfPoint(pt.position, pt.rotation, pt.range);

      manip.drawUi(mx);
      if (manip.manipulate(mx, viewMtx, projMtx)) {
        auto [pos, rot, range] = PointOfMatrix(mx);

        pt.position = pos;
        pt.rotation = rot;
        pt.range = range;

        // This checks for NAN
        assert(pt == pt);
      }
    }
  }

  if (mKmp != nullptr) {
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
}

} // namespace riistudio::lvl
